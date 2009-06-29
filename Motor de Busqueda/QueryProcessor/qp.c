/*
 * File:   qp.c
 * Author: Lucifer
 *
 * Created on 2 de mayo de 2009, 23:15
 */

#include "qp.h"
#include "config.h"
#include "irc.h"
#include "mldap.h"
#include "http.h"

void rutinaDeError(char *string);
SOCKET establecerConexionEscucha(in_addr_t nDireccionIP, in_port_t nPort);
SOCKET establecerConexionServidor(in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *their_addr);
int atenderConsulta(SOCKET sockfd, ldapObj ldap, int cantidadConexiones);
int conectarQueryManager (in_addr_t nDireccionIP, in_port_t nPort);

configuracion config;

int main()
{
    SOCKET sockQP;
    
    ldapObj ldap;
    
    fd_set fdMaestro;
    fd_set fdLectura;
    struct timeval timeout;
    int fdMax, cli, cantidadConexiones;

    fdMax = cli = cantidadConexiones = 0;
    
    /*Lectura de Archivo de Configuracion*/
    printf("Se leera archivo de configuracion. ");
    if (leerArchivoConfiguracion(&config) < 0)
      rutinaDeError("Lectura Archivo de configuracion");
    printf("Leido OK.\n");

    /*Se realiza la conexion a la base ldap*/
    printf("Se realizara la conexion a ldap. ");
    if (establecerConexionLDAP(&ldap, config) < 0)
      rutinaDeError("No se pudo establecer la conexion LDAP.");	
    printf("Conectado OK\n");

    /*Se realiza handshake con el QM, enviando el tipo de recurso*/
    printf("Se realizara el Handshake con Query Manager. ");
    if (conectarQueryManager(config.ipQM, config.puertoQM) < 0)
        rutinaDeError("Conectar Query Manager");
    printf("Realizado OK.\n");

    /*Se establece la conexion de escucha*/
    printf("Se establecera conexion de escucha. ");
    if ((sockQP = establecerConexionEscucha(INADDR_ANY, config.puerto)) < 0)
      rutinaDeError("Establecer conexion de escucha");
    printf("Conexion establecida OK.\n");

    putchar('\n');
    FD_ZERO (&fdMaestro);
    FD_SET(sockQP, &fdMaestro);
    fdMax = sockQP;
    
    while (1)
    {
        int rc, desc_ready;
        
        timeout.tv_sec = 60*3;
        timeout.tv_usec = 0;

        FD_ZERO(&fdLectura);
        memcpy(&fdLectura, &fdMaestro, sizeof(fdMaestro));

        printf("Esperando nuevas peticiones...\n");
        rc = select(fdMax+1, &fdLectura, NULL, NULL, &timeout);

        if (rc < 0)
            rutinaDeError("select");
        if (rc == 0)
        {
        /*SELECT TIMEOUT*/
        }
        if (rc > 0)
        {
            desc_ready = rc;
            for (cli = 0; cli < fdMax+1 && desc_ready > 0; cli++)
            {
                if (FD_ISSET(cli, &fdLectura))
                {
                    desc_ready--;
                    if (cli == sockQP)
                    /*Nueva conexion detectada -> Thread en front end*/
                    {
                        /*Cliente*/
                        SOCKET sockCliente;
                        SOCKADDR_IN dirCliente;
                        int nAddrSize = sizeof(dirCliente);
                        char descriptorID[DESCRIPTORID_LEN];
                        int mode = 0x00;

                        memset(descriptorID, '\0', DESCRIPTORID_LEN);
                       
                        /*Acepta la conexion entrante. Thread en el front end*/
                        sockCliente = accept(sockQP, (SOCKADDR *) &dirCliente, &nAddrSize);
                        if (sockCliente == INVALID_SOCKET)
                        {
                            perror("No se pudo conectar cliente");
                            continue;
                        }

                        printf("Conexion aceptada de %s.\n", inet_ntoa(dirCliente.sin_addr));

                        /*Recibe el IRC con el permiso de peticion de atender request*/
                        printf("Se recibira Handshake. ");
                        if (ircRequest_recv (sockCliente, NULL, descriptorID, &mode) < 0)
                        {
                            printf("Error.\n\n");
                            close(sockCliente);
                            continue;
                        }
                        printf("Recibido OK.\n");

                        /*Si se pueden realizar conexiones*/
                        if (mode == IRC_REQUEST_POSIBLE)
                        {
                            printf("Se ha recibido un permiso de peticion.\n");

                            /*Si no hay conexiones disponibles*/
                            if (!(cantidadConexiones < config.cantidadConexiones))
                                mode = IRC_RESPONSE_NOT_POSIBLE;
                            else
                                mode = IRC_RESPONSE_POSIBLE;                        

                            /*Responde el IRC con codigo de error*/
                            printf("Se respondera permiso de peticion. ");
                            if (ircResponse_send(sockCliente, descriptorID, NULL, 0, mode) < 0)
                            {
                                printf("Error.\n\n");
                                close(sockCliente);
                                continue;
                            }
                            printf("Respondido OK.\n");

                            /*Si no fue posible cierra conexion.*/
                            if (mode == IRC_RESPONSE_NOT_POSIBLE)
                            {
                                printf("Conexion a sido rechazada.\n\n");
                                close(sockCliente);
                            }
                            /*Si fue posible agrega para atender su request*/
                            else
                            {
                                /*Agrega cliente y actualiza max*/
                                FD_SET (sockCliente, &fdMaestro);
                                if (sockCliente > fdMax)
                                        fdMax = sockCliente;
                                cantidadConexiones++;
                                printf("Conexion a sido aceptada.\n\n");
                            }
                        }
                        else if(mode == IRC_REQUEST_IS_ALIVE)
                        {
                            printf("Se a recibido una confirmacion de vida.\n");

                            mode = IRC_RESPONSE_IS_ALIVE;

                            /*Se respondera que se sigue conectado*/
                            printf("Se respondera confirmacion de vida. ");
                            if (ircResponse_send(sockCliente, descriptorID, NULL, 0, mode) < 0)
                            {
                                printf("Error.\n\n");
                                close(sockCliente);
                                continue;
                            }
                            printf("Respondido OK.\n");

                            close(sockCliente);
                        }
                    }
                    else
                    /*Cliente detectado -> esta enviando consultas*/
                    {
                        int control;

                        printf("Se atendera peticion del Cliente.\n");

                        control = atenderConsulta(cli, ldap, cantidadConexiones);

                        if (control == 2)
                            printf("Se a llegado al tope conexiones, se rechaza el pedido.");
                        else if (control == 1)
                            printf("Se a solicitado un recurso que no atiende este Query Processor.");
                        else if (control == 0)
                                  printf("Atencion de cliente finalizada satisfactoriamente.");
                        else if (control < 0)
                                printf("Hubo un error al atender Cliente.");
                        printf(" Se cierra conexion.\n\n");
                   
                        /*Eliminar cliente y actualizar nuevo maximo*/
                        close(cli);
                        FD_CLR(cli, &fdMaestro);
                        if (cli == fdMax)
                            while (FD_ISSET(fdMax, &fdMaestro) == 0)
                                fdMax--;
                        cantidadConexiones--;
                    }
                }
            }
        }
    }

    /*Cierra conexiones de Sockets*/
    for (cli = 0; cli < fdMax+1; ++cli) /*cerrar descriptores*/
        close(cli);

    /*Cierra conexiones LDAP*/
    ldap.sessionOp->endSession(ldap.session);
    
    /*Libero los objetos de operaciones*/
    freeLDAPSession(ldap.session);
    freeLDAPContext(ldap.context);
    freeLDAPContextOperations(ldap.ctxOp);
    freeLDAPSessionOperations(ldap.sessionOp);

    return 0;
}

/*      
Descripcion: Atiende el cliente, recibe busca en LDAP y responde.
Ultima modificacion: Moya, Lucio
Recibe: Socket y estructura LDAP.
Devuelve: ok? Socket del servidor: socket invalido.
*/

int atenderConsulta(SOCKET sockCliente, ldapObj ldap, int cantidadConexiones)
{ 
	/*IRC/IPC*/
	msgGet getInfo;
	char descriptorID[DESCRIPTORID_LEN];
	int mode = 0x00;
	void *resultados = NULL;
	unsigned long len = 0;
	PLDAP_RESULT_SET resultSet = NULL;
        unsigned int cantBloques = 0;

        memset(&getInfo, '\0', sizeof(getInfo));
        memset(descriptorID, '\0', sizeof(descriptorID));

	/*Recibe las palabras a buscar*/
        printf("Se recibiran las palabras a buscar. ");
	if (ircRequest_recv (sockCliente, (void *) &getInfo, descriptorID, &mode) < 0)
	{
            printf("Error.\n");
            return -1;
	}
        printf("Recibidas OK.\n");

        if (mode != IRC_REQUEST_UNICOQP)
        {
            /*Si no hay concordancia con los tipos de recursos que se enviaron y que se atienden*/
            if ( (config.tipoRecurso == 1 && mode != IRC_REQUEST_ARCHIVOS) ||
                    (config.tipoRecurso == 0 && (mode != IRC_REQUEST_HTML || mode != IRC_REQUEST_CACHE)) )
            {
                printf("El recurso pedido no es atendido por este Query Processor.\n");

                /*Envia el IRC con codigo de error*/
                printf("Se enviara Internal Service Error. ");
                if (ircResponse_send(sockCliente, descriptorID, NULL, 0, IRC_RESPONSE_ERROR) < 0)
                {
                    printf("Error.\n");
                    return -1;
                }
                printf("Enviado OK.\n");
                return 1;
            }
        }
        else
        {
            if (getInfo.searchType == WEB)
                mode = IRC_REQUEST_HTML;
            else
                mode = IRC_REQUEST_ARCHIVOS;
        }
        
	/*Realiza la busqueda*/
        printf("Se realizara la busqueda en ldap. ");
	if ((resultSet = consultarLDAP(&ldap, getInfo.queryString)) == NULL)
	{
            printf("Error al consultar ldap.\n");
            return -1;
	}

	/*Prepara la informacion a enviar por el IRC*/
	if ((armarPayload(resultSet, &resultados, mode, &cantBloques)) < 0)
	{
            printf("Error al armar payload.\n");
            ldapFreeResultSet(resultSet);
            return -1;
	}

        printf("Busqueda realizada OK.\n");

        /*Indentifica el tipo de busqueda*/
	if (mode == IRC_REQUEST_HTML)
	{
            len = (sizeof(so_URL_HTML))*cantBloques;
	    mode = IRC_RESPONSE_HTML;
	}	
        else if (mode == IRC_REQUEST_ARCHIVOS)
	{
            len = (sizeof(so_URL_Archivos))*cantBloques;
	    mode = IRC_RESPONSE_ARCHIVOS;
	}
        else if (mode == IRC_REQUEST_CACHE)
	{
            len = sizeof(hostsCodigo);
	    mode = IRC_RESPONSE_CACHE;
	}
        
	/*Envia el IRC con los datos encontrados*/
        printf("Se enviara respuesta. ");
	if (ircResponse_send(sockCliente, descriptorID, resultados, len, mode) < 0)
	{
            printf("Error.\n");
            return -1;
	}
        printf("Respuesta enviada OK.\n");

	free(resultados);
	
	return 0;
}

int conectarQueryManager (in_addr_t nDireccionIP, in_port_t nPort)
{
    SOCKET sockQM;
    SOCKADDR_IN their_addr;
    char buffer[BUF_SIZE];
    char descID[DESCRIPTORID_LEN];
    int mode = 0x00;

    memset(buffer, '\0', sizeof(buffer));

    printf("Conectando Query Manager. ");
    sockQM = establecerConexionServidor(nDireccionIP, nPort, &their_addr);
    if (sockQM == INVALID_SOCKET)
    {
        printf("Error.\n");
        return -1;
    }
    printf("OK.\n");

    sprintf(buffer, "%s:%d-%d", inet_ntoa(*(IN_ADDR *) &config.ip), ntohs(config.puerto), config.tipoRecurso);

    printf("Realizando Handshake. ");
    if (ircRequest_send(sockQM, buffer, sizeof(buffer), descID, IRC_HANDSHAKE_QP) < 0)
    {
        printf("Error.\n");
        close(sockQM);
        return -1;
    }
    printf("OK.\n");

    close(sockQM);

    return 0;
}

SOCKET establecerConexionServidor(in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *their_addr)
{
    SOCKET sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        rutinaDeError("socket");

    their_addr->sin_family = AF_INET;
    their_addr->sin_port = nPort;
    their_addr->sin_addr.s_addr = nDireccionIP;
    memset(&(their_addr->sin_zero),'\0',8);

    /*if (connect(sockfd, (struct sockaddr *)their_addr, sizeof(struct sockaddr)) == -1)
        rutinaDeError("connect");*/

    while ( connect (sockfd, (SOCKADDR *)their_addr, sizeof(SOCKADDR)) == -1 && errno != EISCONN )
        if ( errno != EINTR )
        {
            perror("connect");
            close(sockfd);
            return -1;
    	}

    return sockfd;
}

/*      
Descripcion: Establece la conexion del servidor web a la escucha en un puerto y direccion ip.
Ultima modificacion: Scheinkman, Mariano
Recibe: Direccion ip y puerto a escuchar.
Devuelve: ok? Socket del servidor: socket invalido.
*/
SOCKET establecerConexionEscucha(in_addr_t nDireccionIP, in_port_t nPort)
{
    SOCKET sockfd;

    /*Obtiene un socket*/
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd != INVALID_SOCKET)
    {
        SOCKADDR_IN addrServidorWeb; /*Address del Web server*/
        char yes = 1;
        /*int NonBlock = 1;*/

        /*Impide el error "addres already in use" y setea non blocking el socket*/
        if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1)
            rutinaDeError("Setsockopt");

        /*if (ioctlsocket(sockfd, FIONBIO, &NonBlock) == SOCKET_ERROR)
        {
        rutinaDeError("ioctlsocket");
        }*/

        addrServidorWeb.sin_family = AF_INET;
        addrServidorWeb.sin_addr.s_addr = nDireccionIP;
        addrServidorWeb.sin_port = nPort;
        memset((&addrServidorWeb.sin_zero), '\0', 8);

        /*Liga socket al puerto y direccion*/
        if ((bind (sockfd, (SOCKADDR *) &addrServidorWeb, sizeof(SOCKADDR_IN))) != SOCKET_ERROR)
        {
            /*Pone el puerto a la escucha de conexiones entrantes*/
            if (listen(sockfd, SOMAXCONN) == -1)
                rutinaDeError("Listen");
            else
            return sockfd;
        }
        else
            rutinaDeError("Bind");
    }

    return INVALID_SOCKET;
}

void rutinaDeError(char* error)
{
    printf("\r\nError: %s\r\n", error);
    printf("Codigo de Error: %d\r\n\r\n", errno);
    perror(error);
    exit(EXIT_FAILURE);
}
