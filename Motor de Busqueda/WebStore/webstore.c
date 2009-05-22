/* 
 * File:   webstore.c
 * Author: marianoyfer
 *
 * Created on 15 de mayo de 2009, 11:46
 */
#include "webstore.h"
#include "config.h"

void rutinaDeError(char* error);
SOCKET establecerConexionEscucha(in_addr_t nDireccionIP, in_port_t nPort);
int atenderCrawler(SOCKET sockfd);

configuracion config;

/*
 * 
 */
int main(int argc, char** argv)
{

    SOCKET sockWebStore;
    fd_set fdMaestro;
    fd_set fdLectura;
    struct timeval timeout;
    int fdMax, cli;

    /*Lectura de Archivo de Configuracion*/
    if (leerArchivoConfiguracion(&config) != 0)
       rutinaDeError("Lectura Archivo de configuracion");

    /*Se establece conexion a puerto de escucha*/
    if ((sockWebStore = establecerConexionEscucha(INADDR_ANY, config.puertoL)) == INVALID_SOCKET)
       rutinaDeError("Socket invalido");

    FD_ZERO (&fdMaestro);
    FD_SET(sockWebStore, &fdMaestro);
    fdMax = sockWebStore;

    while (1)
    {
        int rc, desc_ready;

        timeout.tv_sec = 60*3;
        timeout.tv_usec = 0;

        FD_ZERO(&fdLectura);
        memcpy(&fdLectura, &fdMaestro, sizeof(fdMaestro));

        printf("Se comienzan a escuchar posibles Crawlers.\n\n");
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
                    if (cli == sockWebStore)
                    /*Nueva conexion detectada -> Crawler*/
                    {
                        /*Cliente*/
                        SOCKET sockCrawler;
                        SOCKADDR_IN dirCrawler;
                        int nAddrSize = sizeof(dirCrawler);

                        /*Acepta la conexion entrante. Thread en el front end*/
                        sockCrawler = accept(sockWebStore, (SOCKADDR *) &dirCrawler, &nAddrSize);
                        if (sockCrawler == INVALID_SOCKET)
                        {
                            perror("No se pudo conectar Crawler");
                            continue;
                        }

                        /*Agrega cliente y actualiza max*/
                        FD_SET (sockCrawler, &fdMaestro);
                        if (sockCrawler > fdMax)
                                fdMax = sockCrawler;

                        printf ("Conexion aceptada de %s.\n", inet_ntoa(dirCrawler.sin_addr));
                    }
                    else
                    /*Crawler detectado -> esta enviando consultas*/
                    {
                        if (atenderCrawler(cli) < 0)
                            printf("Hubo un error al atender Crawler.");
                        else
                            printf("Atencion del Crawler finalizada satisfactoriamente.");

                        printf("Se cierra conexion.\n\n");

                        /*Eliminar crawler y actualizar nuevo maximo*/
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

    close(sockWebStore);
    return (EXIT_SUCCESS);
}

int atenderCrawler(SOCKET sockfd)
{
    /*HACER*/
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
