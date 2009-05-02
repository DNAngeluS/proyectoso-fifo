/* 
 * File:   frontend.c
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 18:56
 */

#include "frontend.h"
#include "config.h"
#include "irc.h"
#include "http.h"


void rutinaDeError(char *string);

SOCKET establecerConexionEscucha(in_addr_t nDireccionIP, in_port_t nPort);
SOCKET establecerConexionQP(in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR *dir);

int rutinaCrearThread(void (*funcion)(void *), SOCKET sockfd);
void rutinaAtencionCliente(void *args);


SOCKET sockQP;
SOCKADDR_IN dirQP;
configuracion config;
/*
 * 
 */
int main(int argc, char** argv) {

    SOCKET sockFrontEnd;
    SOCKET sockCliente;

    SOCKADDR_IN dirFrontEnd;
    SOCKADDR_IN dirCliente;


    /*Lectura de Archivo de Configuracion*/
    if (leerArchivoConfiguracion(&config) != 0)
        rutinaDeError("Lectura Archivo de configuracion");


    /*Se establece conexion con el Query Procesor*/
    if ((sockQP = establecerConexionQP(config.ipQP, config.puertoQP, (SOCKADDR *)&dirQP)) == INVALID_SOCKET)
        rutinaDeError("Socket invalido");

    /*Se establece conexion a puerto de escucha*/
    if ((sockFrontEnd = establecerConexionEscucha(INADDR_ANY, config.puertoL)) == INVALID_SOCKET)
        rutinaDeError("Socket invalido");

    while(1)
    {
        int nAddrSize = sizeof(dirCliente);

        /*Acepta la conexion entrante*/
	sockCliente = accept(sockFrontEnd, (SOCKADDR *) &dirCliente, &nAddrSize);

        if (sockCliente == INVALID_SOCKET)
            continue;

        printf ("Conexion aceptada de %s:%d.\r\n\r\n", inet_ntoa(dirCliente.sin_addr),
                                                        ntohs(dirCliente.sin_port));
        if (enviarHtml (sockCliente) < 0)
            continue;
        rutinaCrearThread(rutinaAtencionCliente, sockCliente);
        
    }

    close(sockQP);
    close(sockFrontEnd);
    return (EXIT_SUCCESS);
}

void rutinaAtencionCliente(void *args)
{
    SOCKET sockCliente = (SOCKET) args;
    msgGet getInfo;

    /*Recibir el Http GET*/
    if (httpGet_recv(sockCliente, &getInfo) < 0)
    {
        printf("Error al recibir HTTP GET. Se cierra conexion.\n\n");
        close(sockCliente);
        return;
    }
    /*Enviar consulta a QP*/
    if (ircRequest_send(sockQP, getInfo.palabras) < 0)
    {
        printf("Error al enviar consulta a QP. Se cierra conexion.\n\n");
        close(sockCliente);
        return;
    }
    if (ircResponse_recv(sockQP) < 0)
    {
        close(sockCliente);
        return;
    }

}

int rutinaCrearThread(void (*funcion)(void *), SOCKET sockfd)
{
    /*HACER*/
}




/*
Descripcion: Atiende error cada vez que exista en alguna funcion y finaliza proceso.
Ultima modificacion: Scheinkman, Mariano
Recibe: Error detectado.
Devuelve: int rutinaCrearThread(void (*funcion)(void *), SOCKET sockfd);-
 */
void rutinaDeError(char* error) {
    printf("\r\nError: %s\r\n", error);
    printf("Codigo de Error: %d\r\n\r\n", errno);
    exit(EXIT_FAILURE);
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


/*
Descripcion: Establece la conexion del front-end con el Query Procesor.
Ultima modificacion: Scheinkman, Mariano
Recibe: Direccion ip y puerto del QP.
Devuelve: ok? Socket del servidor: socket invalido.
*/
SOCKET establecerConexionQP(in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR *dir)
{
    SOCKET sockfd;
    SOCKET sockQP;

    SOCKADDR_IN dest_addr;
    
    if (sockfd= socket(AF_INET, SOCK_STREAM, 0) == -1)
        return INVALID_SOCKET;

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = nPort;
    dest_addr.sin_addr.s_addr = nDireccionIP;
    memset(&(dest_addr.sin_zero), '\0', 8);

    if ((sockQP = connect(sockfd, dir, sizeof(SOCKADDR_IN))) == -1)
        return INVALID_SOCKET;

    return sockQP;
}



















