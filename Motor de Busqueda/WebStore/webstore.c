/* 
 * File:   webstore.c
 * Author: marianoyfer
 *
 * Created on 15 de mayo de 2009, 11:46
 */
#include "webstore.h"
#include "config.h"

void rutinaDeError(char* error);
void signalHandler(int sig);
SOCKET establecerConexionEscucha(in_addr_t nDireccionIP, in_port_t nPort);

int atenderCrawler(SOCKET sockfd);

int childID=0;
configuracion config;
pthread_mutex_t condition_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  condition_start  = PTHREAD_COND_INITIALIZER;

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

    /*Se agrega a SIGCHLD como señal a atenderse*/
    if (signal(SIGCHLD, signalHandler) == SIG_ERR)
        rutinaDeError("signal");

    /*Lectura de Archivo de Configuracion*/
    if (leerArchivoConfiguracion(&config) != 0)
        rutinaDeError("Lectura Archivo de configuracion");

    /*Se establece conexion a puerto de escucha*/
    if ((sockWebStore = establecerConexionEscucha(INADDR_ANY, config.puertoL)) == INVALID_SOCKET)
        rutinaDeError("Socket invalido");

    /*Se atiendera UNA UNICA VEZ la señal SIGUSR1, por eso se usa signal()
     *que una vez atendida reinicia el handler original de la señal
     *que para SIGURS1 no existe, justo lo que queremos
     */
    if (signal(SIGUSR1, signalHandler) == SIG_ERR)
        rutinaDeError("signal");


    /*Se espera por el SIGUSR1 para comenzar a procesar*/
    printf("Esperando SIGUSR1...\n");
    pthread_mutex_lock( &condition_mutex );
    while(childID != 0)
    {
        pthread_cond_wait( &condition_start, &condition_mutex );
    }
    pthread_mutex_unlock( &condition_mutex );

    /*Se inicializan datos para el select()*/
    FD_ZERO (&fdMaestro);
    FD_SET(sockWebStore, &fdMaestro);
    fdMax = sockWebStore;

    while (1)
    {
        int rc, desc_ready;

        /*Se establece un tiempo de timeout para select()*/
        timeout.tv_sec = config.tiempoNuevaConsulta;
        timeout.tv_usec = 0;

        FD_ZERO(&fdLectura);
        memcpy(&fdLectura, &fdMaestro, sizeof(fdMaestro));

        printf("Se comienzan a escuchar posibles Crawlers.\n\n");
        rc = select(fdMax+1, &fdLectura, NULL, NULL, &timeout);

        if (rc < 0)
        /*Error en select()*/
            rutinaDeError("select");
        if (rc == 0)
        /*Select timeout*/
        {
            kill(childID, SIGUSR1);
        }
        if (rc > 0)
        /*Se detectaron sockets con actividad*/
        {
            desc_ready = rc;
            
            /*Se recorren todos los posibles sockets buscando los que tuvieron actividad*/
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
                        /*Atiende al Crawler*/
                        if (atenderCrawler(cli) < 0)
                            printf("Hubo un error al atender Crawler.");
                        else
                            printf("Atencion del Crawler finalizada satisfactoriamente.");

                        printf("Se cierra conexion.\n\n");

                        /*Eliminar Crawler y actualizar nuevo maximo*/
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

void signalHandler(int sig)
{
    switch(sig)
    {
        case SIGUSR1:
        {
            char *argv[] = {"crawler-create", "NULL", "NULL", "NULL", "NULL"};
            int i;

            for (i=1;i<=3;i++)
                argv[i] = (char *) malloc (sizeof(char)*20);

            sprintf(argv[1], "%d", config.ipWebServer);
            sprintf(argv[2], "%d", config.puertoWebServer);
            sprintf(argv[3], "%d", config.tiempoNuevaConsulta);

            pthread_mutex_lock( &condition_mutex );
            childID = fork();
  
            if ((childID) < 0)
            {
                /*for (i=1;i<=3;i++)
                    free(argv[i]);*/
                rutinaDeError("fork. Creacion de proceso crawler-create");
            }
            else if (childID == 0)
            {
                execv("crawler-create", argv);
                
                /*Si ejectua esto fue porque fallo execv*/
                rutinaDeError("execv");
            }
            else if (childID > 0)
            {
                printf("Se a creado proceso crawler-create");
                for (i=1;i<=3;i++)
                    free(argv[i]);
                pthread_cond_signal( &condition_start );
                pthread_mutex_unlock( &condition_mutex );
            }
        }
            break;
            
        case SIGCHLD:
            while(waitpid(-1, NULL, WNOHANG) > 0);
            break;

        /*case SIGINT:
         *   //if (childID != 0 VOLVER A SELECT() else sigint
         *   break;
         */
    }

    if (sig != SIGUSR1)
        if (signal(sig, signalHandler) == SIG_ERR)
            rutinaDeError("signal");
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
