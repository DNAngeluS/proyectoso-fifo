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
#include "log.h"

void rutinaDeError(char *string, int log);
SOCKET establecerConexionEscucha(in_addr_t nDireccionIP, in_port_t nPort);
SOCKET establecerConexionServidor(in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *their_addr);
int atenderConsulta(SOCKET sockfd, ldapObj *ldap, int cantidadConexiones);
int conectarQueryManager (in_addr_t nDireccionIP, in_port_t nPort);

configuracion config;
mutex_t logMutex;


int main()
{
    SOCKET sockQP;
    ldapObj ldap;
    
    fd_set fdMaestro;
    fd_set fdLectura;
    struct timeval timeout;
    int fdMax, cli, cantidadConexiones;
    mode_t modeOpen = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    fdMax = cli = cantidadConexiones = 0;

    /*Se inicializa el mutex*/
    mutex_init(&logMutex, USYNC_THREAD, NULL);

    WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Inicio de ejecucion", "INFO");
    
   /*Lectura de Archivo de Configuracion*/
    WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Se leera archivo de configuracion", "INFO");
    if (leerArchivoConfiguracion(&config) != 0)
       rutinaDeError("Lectura Archivo de configuracion", config->log);
    WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Leido OK", "INFOFIN");

    /*Se realiza la conexion a la base ldap*/
    WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Se realizara la conexion a ldap", "INFO");
    if (establecerConexionLDAP(&ldap, config) < 0)
      rutinaDeError("No se pudo establecer la conexion LDAP.", config->log);
    WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Conectado OK", "INFOFIN");

    /*Se realiza handshake con el QM, enviando el tipo de recurso*/
    WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Se realizara el Handshake con Query Manager", "INFO");
    if (conectarQueryManager(config.ipQM, config.puertoQM) < 0)
        rutinaDeError("Conectar Query Manager", config->log);
    WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Realizado OK", "INFOFIN");

    /*Se establece la conexion de escucha*/
    WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Se establecera la conexion de escucha", "INFO");
    if ((sockQP = establecerConexionEscucha(INADDR_ANY, config.puerto)) < 0)
      rutinaDeError("Establecer conexion de escucha", config->log);
    WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Realizado OK", "INFOFIN");

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

        WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Esperando nuevas peticiones...", "INFOFIN");
        putchar('\n');
        rc = select(fdMax+1, &fdLectura, NULL, NULL, &timeout);

        if (rc < 0)
            rutinaDeError("select", config->log);
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
                        char text[60];

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
                            WriteLog(config->log, "Query Processor", getpid(), thr_self(), "No se pudo conectar cliente", "ERROR");
                            continue;
                        }

                        sprintf(text, "Conexion aceptada de %s", inet_ntoa(dirCliente.sin_addr));
                        WriteLog(config->log, "Query Processor", getpid(), thr_self(), text, "INFOFIN");

                        /*Recibe el IRC con el permiso de peticion de atender request*/
                        WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Se recibira handshake", "INFO");
                        if (ircRequest_recv (sockCliente, NULL, descriptorID, &mode) < 0)
                        {
                            WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Error", "ERROR");
                            putchar('\n');
                            close(sockCliente);
                            continue;
                        }
                        WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Recibido OK", "INFOFIN");

                        /*Si se pueden realizar conexiones*/
                        if (mode == IRC_REQUEST_POSIBLE)
                        {
                            WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Se a recibido un permiso de peticion", "INFOFIN");

                            /*Si no hay conexiones disponibles*/
                            if (!(cantidadConexiones < config.cantidadConexiones))
                                mode = IRC_RESPONSE_NOT_POSIBLE;
                            else
                                mode = IRC_RESPONSE_POSIBLE;                        

                            /*Responde el IRC con codigo de error*/
                            WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Se respondera permiso de peticion", "INFO");
                            if (ircResponse_send(sockCliente, descriptorID, NULL, 0, mode) < 0)
                            {
                                WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Error", "ERROR");
                                close(sockCliente);
                                continue;
                            }
                            WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Respondido OK", "INFOFIN");

                            /*Si no fue posible cierra conexion.*/
                            if (mode == IRC_RESPONSE_NOT_POSIBLE)
                            {
                                WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Conexion a sido rechazada", "INFOFIN");
                                putchar('\n');
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
                                WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Conexion a sido aceptada", "INFOFIN");
                                putchar('\n');
                            }
                        }
                        else if(mode == IRC_REQUEST_IS_ALIVE)
                        {
                            /*printf("Se a recibido una confirmacion de vida.\n");*/

                            mode = IRC_RESPONSE_IS_ALIVE;

                            /*Se respondera que se sigue conectado*/
                            /*printf("Se respondera confirmacion de vida. ");*/
                            if (ircResponse_send(sockCliente, descriptorID, NULL, 0, mode) < 0)
                            {
                                /*printf("Error.\n\n");*/
                                close(sockCliente);
                                continue;
                            }
                            /*printf("Respondido OK.\n");*/

                            close(sockCliente);
                        }
                    }
                    else
                    /*Cliente detectado -> esta enviando consultas*/
                    {
                        int control;
                        char text[60];

                        WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Se atendera la peticion del cliente", "INFOFIN");

                        control = atenderConsulta(cli, &ldap, cantidadConexiones);

                        if (control == 2)
                            sprintf(text, "Se a llegado al tope conexiones, se rechaza el pedido.");
                        else if (control == 1)
                            sprintf(text, "Se a solicitado un recurso que no atiende este Query Processor.");
                        else if (control == 0)
                            sprintf(text, "Atencion de cliente finalizada satisfactoriamente.");
                        else if (control < 0)
                            sprintf(text, "Hubo un error al atender Cliente.");
                        strcat(text, " Se cierra conexion");

                        WriteLog(config->log, "Query Processor", getpid(), thr_self(), text, control<0? "ERROR": "INFOFIN");
                        putchar('\n');
                   
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

	/*Finalizo el mutex*/
    mutex_destroy(&logMutex);
	close(config->log);

    return 0;
}

/*      
Descripcion: Atiende el cliente, recibe busca en LDAP y responde.
Ultima modificacion: Moya, Lucio
Recibe: Socket y estructura LDAP.
Devuelve: ok? Socket del servidor: socket invalido.
*/

int atenderConsulta(SOCKET sockCliente, ldapObj *ldap, int cantidadConexiones)
{ 
	/*IRC/IPC*/
	msgGet getInfo;
	char descriptorID[DESCRIPTORID_LEN];
	int mode = 0x00;
	void *resultados = NULL;
	unsigned long len = 0;
	PLDAP_RESULT_SET resultSet = NULL;
  unsigned int cantBloques = 0;
	struct timeval timeout = {config.tiempoDemora, 0};

        memset(&getInfo, '\0', sizeof(getInfo));
        memset(descriptorID, '\0', sizeof(descriptorID));

	/*Recibe las palabras a buscar*/
  WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Se recibiran las palabras a buscar", "INFO");
	if (ircRequest_recv (sockCliente, (void *) &getInfo, descriptorID, &mode) < 0)
	{
            WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Error", "ERROR");
            return -1;
	}
        WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Recibidas OK", "INFOFIN");

        if (mode != IRC_REQUEST_UNICOQP)
        {
            /*Si no hay concordancia con los tipos de recursos que se enviaron y que se atienden*/
            if ( (config.tipoRecurso == RECURSO_ARCHIVOS && mode != IRC_REQUEST_ARCHIVOS) ||
                    (config.tipoRecurso == RECURSO_WEB && !(mode == IRC_REQUEST_HTML || mode == IRC_REQUEST_CACHE)) )
            {
                WriteLog(config->log, "Query Processor", getpid(), thr_self(), "El recurso pedido no es atendido por este Query Processor", "INFOFIN");

                /*Envia el IRC con codigo de error*/
                WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Se enviara Internal Service Error", "INFO");
                if (ircResponse_send(sockCliente, descriptorID, NULL, 0, IRC_RESPONSE_ERROR) < 0)
                {
                    WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Error", "ERROR");
                    return -1;
                }
                WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Recibidas OK", "INFOFIN");
                return 1;
            }
        }
        else
        {
            if (getInfo.searchType == WEB)
                mode = IRC_REQUEST_HTML;
            else if (getInfo.searchType == CACHE)
                mode = IRC_REQUEST_CACHE;
            else
                mode = IRC_REQUEST_ARCHIVOS;
        }
        
	/*Realiza la busqueda*/
        WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Se realizara la busqueda en ldap", "INFO");
	if ((resultSet = consultarLDAP(ldap, getInfo.queryString)) == NULL)
	{
            WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Error al consultar ldap", "ERROR");
            return -1;
	}

	/*Prepara la informacion a enviar por el IRC*/
	if ((armarPayload(resultSet, &resultados, mode, &cantBloques)) < 0)
	{
            WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Error al armar el payload", "ERROR");
            ldapFreeResultSet(resultSet);
            return -1;
	}

        WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Busqueda realizada", "INFOFIN");

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

	/*Demora del tiempo especificado en el archivo de configuracion*/
	select(0,0,0,0, &timeout);
        
	/*Envia el IRC con los datos encontrados*/
        WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Se enviara respuesta", "INFO");
	if (ircResponse_send(sockCliente, descriptorID, resultados, len, mode) < 0)
	{
            WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Error", "ERROR");
            return -1;
	}
        WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Respuesta enviada OK", "INFOFIN");

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

    WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Conectando Query Manager", "INFOFIN");
    sockQM = establecerConexionServidor(nDireccionIP, nPort, &their_addr);
    if (sockQM == INVALID_SOCKET)
    {
        WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Error", "ERROR");
        return -1;
    }
    WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Conexion OK", "INFOFIN");

    sprintf(buffer, "%s:%d-%d+%d", inet_ntoa(*(IN_ADDR *) &config.ip), ntohs(config.puerto), config.tipoRecurso, config.cantidadConexiones);

    WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Realizando Handshake", "INFOFIN");
    if (ircRequest_send(sockQM, buffer, sizeof(buffer), descID, IRC_HANDSHAKE_QP) < 0)
    {
        WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Error", "ERROR");
        close(sockQM);
        return -1;
    }
    WriteLog(config->log, "Query Processor", getpid(), thr_self(), "Handshake OK", "INFOFIN");

    close(sockQM);

    return 0;
}

SOCKET establecerConexionServidor(in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *their_addr)
{
    SOCKET sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return -1;

    their_addr->sin_family = AF_INET;
    their_addr->sin_port = nPort;
    their_addr->sin_addr.s_addr = nDireccionIP;
    memset(&(their_addr->sin_zero),'\0',8);

    /*if (connect(sockfd, (struct sockaddr *)their_addr, sizeof(struct sockaddr)) == -1)
        return -1;*/

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
           return -1;

        /*if (ioctlsocket(sockfd, FIONBIO, &NonBlock) == SOCKET_ERROR)
        {
        return -1;
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
                return -1;
            else
            return sockfd;
        }
        else
            return -1;
    }

    return INVALID_SOCKET;
}

void rutinaDeError(char* error, int log)
{
    printf("\r\nError: %s\r\n", error);
    printf("Codigo de Error: %d\r\n\r\n", errno);
    perror(error);

    /*Mutua exclusion*/
	if (log != 0)
	{
    	WriteLog(log, "Query Processor", getpid(), thr_self(), error, "ERROR");
		close(log);
	}
	
    exit(EXIT_FAILURE);
}
