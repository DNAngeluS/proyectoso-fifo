/*
 * File:   qp.c
 * Author: Lucifer
 *
 * Created on 2 de mayo de 2009, 23:15
 */

#include "qp.h"
#include "config.h"
#include "irc.h"
#include "LdapWrapper.h"
#include "mldap.h"
#include "http.h"

void rutinaDeError(char *string);
SOCKET establecerConexionEscucha(in_addr_t nDireccionIP, in_port_t nPort);
int atenderConsulta(SOCKET sockfd, ldapObj ldap);

int main()
{
    SOCKET sockQP;
    
    configuracion config;
    ldapObj ldap;
    
    fd_set fdMaestro;
    fd_set fdLectura;
    struct timeval timeout;
    int fdMax, cli;
    
    /*Lectura de Archivo de Configuracion*/
    if (leerArchivoConfiguracion(&config) < 0)
      rutinaDeError("Lectura Archivo de configuracion");

    if(establecerConexionLDAP(&ldap, config) < 0)
      rutinaDeError("No se pudo establecer la conexion LDAP.");	
    printf("Conexion establecida con LDAP\n");
    
    if ((sockQP = establecerConexionEscucha(INADDR_ANY, config.puerto)) < 0)
      rutinaDeError("Establecer conexion de escucha");
    printf("QUERY PROCESSOR. Conexion establecida.\n");
    
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

        printf("Se comienzan a escuchar pedidos.\n\n");
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

                        memset(descriptorID, '\0', DESCRIPTORID_LEN);

                        /*Acepta la conexion entrante. Thread en el front end*/
                        sockCliente = accept(sockQP, (SOCKADDR *) &dirCliente, &nAddrSize);
                        if (sockCliente == INVALID_SOCKET)
                        {
                            perror("No se pudo conectar cliente");
                            continue;
                        }

                        /*Agrega cliente y actualiza max*/
                        FD_SET (sockCliente, &fdMaestro);
                        if (sockCliente > fdMax)
                                fdMax = sockCliente;

                        printf ("Conexion aceptada de %s.\n", inet_ntoa(dirCliente.sin_addr));
                    }
                    else
                    /*Cliente detectado -> esta enviando consultas*/
                    {
                        if (atenderConsulta(cli, ldap) < 0)
                            printf("Hubo un error al atender Cliente.");
                        else
                            printf("Atencion de cliente finalizada satisfactoriamente.");

                        printf("Se cierra conexion.\n\n");
                        /*Eliminar cliente y actualizar nuevo maximo*/
                        close(cli);
                        FD_CLR(cli, &fdMaestro);
                        if (cli == fdMax)
                            while (FD_ISSET(fdMax, &fdMaestro) == 0)
                                fdMax--;
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

int atenderConsulta(SOCKET sockCliente, ldapObj ldap)
{ 
	/*IRC/IPC*/
	msgGet getInfo;
	char descriptorID[DESCRIPTORID_LEN];
	int mode;
	 
	/*Crea las estructuras para enviar el IRC*/
	void *resultados = NULL;
	unsigned long len;  
	PLDAP_RESULT_SET resultSet;
        unsigned int cantBloques = 0;

	/*Recibe las palabras a buscar*/
	if (ircRequest_recv (sockCliente, (void *) &getInfo, descriptorID, &mode) < 0)
	{
            perror("ircRequest_recv");
            return -1;
	}

	/*Realiza la busqueda*/
	if ((resultSet = consultarLDAP(ldap, getInfo.queryString, mode)) == NULL)
	{
            perror("consultarLDAP");
            return -1;
	}

	/*Prepara la informacion a enviar por el IRC*/
	if ((armarPayload(resultSet, &resultados, mode, &cantBloques)) < 0)
	{
            perror("armarPayload");
            return -1;
	}

        /*Indentifica el tipo de busqueda*/
	if (mode == IRC_REQUEST_HTML)       len = (sizeof(so_URL_HTML))*cantBloques;
	if (mode == IRC_REQUEST_ARCHIVOS)   len = (sizeof(so_URL_Archivos))*cantBloques;

	/*Envia el IRC con los datos encontrados*/
	if (ircResponse_send(sockCliente, descriptorID, resultados, len, mode) < 0)
	{
	  perror("ircResponse_send");
	  return -1;
	}

	free(resultados);
	
	return 0;
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
