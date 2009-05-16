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

headerIRC construirIRC(void *objeto);

int main()
{
    SOCKET sockFE;
    SOCKADDR_IN dirCliente;
    configuracion config;
    ldapObj ldap;
    PLDAP_RESULT_SET resultSet;

    /*Lectura de Archivo de Configuracion*/
    if (leerArchivoConfiguracion(&config) != 0)
      rutinaDeError("Lectura Archivo de configuracion");

    if (establecerConexionEscucha(INADDR_ANY, config.puerto) != 0)
      rutinaDeError("Establecer conexion de escucha");

    ldap = establecerConexionLDAP(config);
      /*rutinaDeError("Establecer conexion LDAP");*/

    while(1)
    {
      SOCKET sockCliente;
      int nAddrSize = sizeof(dirCliente);
      msgGet *getInfo;
      char descriptorID[DESCRIPTORID_LEN];

      memset(descriptorID, '\0', DESCRIPTORID_LEN);
      /*Acepta la conexion entrante. Ahora FrontEnd*/
      sockCliente = accept(sockFE, (SOCKADDR *) &dirCliente, &nAddrSize);

      if (sockCliente != INVALID_SOCKET)
          continue;

      printf ("Conexion aceptada de %s:%d.\n\n", inet_ntoa(dirCliente.sin_addr),
                                                        ntohs(dirCliente.sin_port));
      /*Recibe las palabras a buscar*/
      if (ircRequest_recv (sockCliente, (void *) getInfo, descriptorID) < 0)
      {
          close(sockCliente);
          continue;
      }
      /*Crea las estructuras para enviar el IRC*/
      void *resultados = NULL;
      unsigned long len;

      /*Indentifica el tipo de busqueda*/
      if (getInfo->searchType == WEB) len = (sizeof(so_URL_HTML));
      if (getInfo->searchType == IMG) len = (sizeof(so_URL_Archivos));
      if (getInfo->searchType == OTROS) len = (sizeof(so_URL_Archivos));

      /*Realiza la busqueda*/
      resultSet = consultarLDAP(ldap, getInfo->palabras, getInfo->searchType);
      /*Prepara la informacion a enviar por el IRC*/
      resultados = armarPayload(resultSet, getInfo->searchType);
      
      /*envia el IRC con los datos encontrados*/
      if (ircResponse_send(sockCliente, descriptorID, resultados, len) < 0)
      {
          close(sockCliente);
          continue;
      }

      close(sockCliente);
    }


    /*Cierra conexiones de Sockets*/
    close(sockFE);    

    /*Cierra conexiones LDAP*/
    ldap.sessionOp->endSession(ldap.session);
    /*libero los objetos de operaciones*/
    freeLDAPSession(ldap.session);
    freeLDAPContext(ldap.context);
    freeLDAPContextOperations(ldap.ctxOp);
    freeLDAPSessionOperations(ldap.sessionOp);

    return 0;
}

/*Descripcion: Establece la conexion del servidor web a la escucha en un puerto y direccion ip.
Ultima modificacion: Moya, Lucio
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
        int NonBlock = 1;

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
