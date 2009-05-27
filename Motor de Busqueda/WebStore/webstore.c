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

int atenderCrawler(SOCKET sockCrawler);

configuracion config;
volatile sig_atomic_t sigRecibida=0;

/*
 * 
 */
int main()
{
    SOCKET sockWebStore;
    struct sigaction new_action, old_action;
    fd_set fdMaestro;
    fd_set fdLectura;
    struct timeval timeout;
    int fdMax, cli;
    pid_t childID;

    /*Se establecen los valores de la nueva accion para manejar señales*/
    new_action.sa_handler = signalHandler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = SA_RESTART;

    /*Se asigna un handler a las señales, si no se bloqueo su captura antes*/    
    sigaction (SIGCHLD, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN)
        sigaction (SIGCHLD, &new_action, NULL);
    /*sigaction (SIGINT, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN)
        sigaction (SIGINT, &new_action, NULL);*/
    
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
    sigaction (SIGUSR1, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN)
        sigaction (SIGUSR1, &new_action, NULL);


    /*Se inicialian controladores para bloquear hasta recibir SIGUSR1*/
    sigemptyset (&new_action.sa_mask);
    sigaddset (&new_action.sa_mask, SIGUSR1);
    printf("Esperando SIGUSR1... pid(%d)\n", getpid());

    /*Se espera por el SIGUSR1 para comenzar y de aqui en mas no se atendera mas*/
    while (!sigRecibida)
        sigsuspend (&old_action.sa_mask);
    sigprocmask (SIG_BLOCK, &new_action.sa_mask, &old_action.sa_mask);

    if (sigRecibida)
    {
        char ip[20], puerto[20], tiempoNuevaConsulta[20];
        char path[MAX_PATH];

        sprintf(ip, "%s", inet_ntoa(config.ipWebServer));
        sprintf(puerto, "%d", config.puertoWebServer);
        sprintf(tiempoNuevaConsulta, "%d", config.tiempoNuevaConsulta);
        sprintf(path, "%s/crawler-create", getcwd(NULL, 0));

        if ((childID = fork()) < 0)
        {
            rutinaDeError("fork. Creacion de proceso crawler-create");
        }
        else if (childID == 0)
        {
            char *argv[] = {"crawler-create", ip, puerto, tiempoNuevaConsulta, "NULL"};
            execv(path, argv);

            /*Si ejectua esto fue porque fallo execv*/
            perror("execv");
            kill(getppid(), SIGCHLD);
            exit(EXIT_FAILURE);
        }
        else if (childID > 0)
        {
            printf("Se a creado proceso crawler-create.\n\n");
        }
    }

    /*Se inicializan datos para el select()*/
    FD_ZERO (&fdMaestro);
    FD_SET(sockWebStore, &fdMaestro);
    fdMax = sockWebStore;

    printf("PROCESO HIJO: %d\n\n", childID);


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
            printf("Se a enviado señal SIGURS1 a proceso hijo: %d\n", childID);
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
                            printf("Atencion del Crawler finalizada satisfactoriamente.\n");

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

int atenderCrawler(SOCKET sockCrawler)
{
    /*HACER*/

    void *bloque;
    unsigned long bloqueLen;
    int mode;

    if (ircResponse_recv(sockCrawler, &bloque, &bloqueLen, &mode) < 0)
        rutinaDeError("ircResponse_recv");

    if (mode == IRC_ALTA_HTML)
        LDAP_AltaHtml((crawler_URL_HTML *) bloque);

    else if (mode == IRC_MODIFICACION_HTML)
        LDAP_ModificarHtml((crawler_URL_HTML *) bloque);

    else if (mode == IRC_ALTA_ARCHIVOS)
        LDAP_AltaArchivos((crawler_URL_ARCHIVOS *) bloque);

    else if (mode == IRC_MODIFIACION_ARCHIVOS)
        LDAP_ModificarArchivos((crawler_URL_ARCHIVOS *) bloque);

    else
    {
        printf("Inconcistencia en Payload Descriptor.\n");
        return -1;
    }
    return 0;
}

void signalHandler(int sig)
{
    switch(sig)
    {
        case SIGUSR1:
            sigRecibida = 1;
            break;
            
        case SIGCHLD:
            while(waitpid(-1, NULL, WNOHANG) > 0);
            break;
            
        /*case SIGINT:
         *   //if (childID != 0 VOLVER A SELECT() else sigint
         *   break;
         */
    }
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
