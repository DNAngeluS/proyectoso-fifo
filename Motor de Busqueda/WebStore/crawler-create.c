#include "crawler-create.h"
#include "irc.h"

void rutinaDeError(char *error);
void signalHandler(int sig);

int establecerConexionServidorWeb(in_addr_t nDireccion, in_port_t nPort, SOCKADDR_IN *dir);
int EnviarCrawlerCreate(in_addr_t nDireccion, in_port_t nPort);


volatile sig_atomic_t sigRecibida = 0;

int main(int argc, char **argv)
{
    in_addr_t ipWebServer;
    in_port_t puertoWebServer;
    long tiempoMigracion;
    time_t actualTime;
    struct sigaction new_action, old_action;

    if (argc != 4)
        rutinaDeError("Argumentos invalidos");

    /*Inicializa parametros de Web Server conocido y tiempo de nueva Migracion*/
    ipWebServer = inet_addr(argv[1]);
    puertoWebServer = htons(atoi(argv[2]));/*ESTE ES PARA LA PRUEBA*/
    /*puertoWebServer = atoi(argv[2]);*/
    tiempoMigracion = atoi(argv[3]);

    /*Se establecen los valores de la nueva accion para manejar señales*/
    new_action.sa_handler = signalHandler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = SA_RESTART;

    /*Se asigna un handler a la señal, si no se bloqueo su captura antes*/
    sigaction (SIGUSR1, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN)
        sigaction (SIGUSR1, &new_action, NULL);

    /*Se envia pedido de creacion de Crawler al primer WebServer conocido*/
    if (EnviarCrawlerCreate(ipWebServer, puertoWebServer) < 0)
        rutinaDeError("Enviando primer Crawler");

    while (1)
    {
        int i = 0;
        int maxHosts = 0;
        webServerHosts *hosts = NULL;

        /*Se inicialian controladores para bloquear hasta recibir SIGUSR1*/
        sigemptyset (&new_action.sa_mask);
        sigaddset (&new_action.sa_mask, SIGUSR1);
        printf("Esperando SIGUSR1... pid(%d)\n", getpid());

        /*Se espera por el SIGUSR1 para comenzar*/
        sigprocmask (SIG_BLOCK, &new_action.sa_mask, &old_action.sa_mask);
        while (!sigRecibida)
            sigsuspend (&old_action.sa_mask);
        sigprocmask (SIG_UNBLOCK, &new_action.sa_mask, NULL);

        sigRecibida=0;

        /*ESTO ES PARA LA PRUEBA*/
        hosts = (webServerHosts *) malloc(sizeof(webServerHosts));
        maxHosts=1;
        hosts[0].hostIP=ipWebServer;
        hosts[0].hostPort=puertoWebServer;
        hosts[0].uts=time(NULL);
        /***********************/


        /*Se consulta el dir de expiracion y obtiene la tabla actualizada*/
        if (consultarHostsExpiracion(&hosts, &maxHosts) < 0)
            rutinaDeError("consulta hosts expiracion");

        printf("Se consulto directorio de hosts satisfactoriamente.\n");
        printf("Se dispararan Crawlers a hosts desactualizados\n");
        
        while (i < maxHosts)
        /*Para cada hosts en la tabla*/
        {
            long tiempoRestante;
            actualTime = time(NULL);
            tiempoRestante = (actualTime - hosts[i].uts);

            if (tiempoRestante >= tiempoMigracion)
            /*Si el tiempo que paso desde la ultima migracion es mayor al configurado*/
            {
                /*Envia pedido de creacion de Crawler*/
                if (EnviarCrawlerCreate(hosts[i].hostIP, hosts[i].hostPort) < 0)
                {
                    fprintf(stderr, "Error: Instanciacion de un Crawler a %s:%d\n",
                            inet_aton(hosts[i].hostIP), ntohs(hosts[i].hostPort));
                    continue;
                }
                else
                {
                    char ipPuerto[MAX_PATH];

                    sprintf(ipPuerto, "%s:%d", inet_ntoa(hosts[i].hostIP), ntohs(hosts[i].hostPort));

                    printf("Se envio peticion de Crawler a %s.\n", inet_ntoa(hosts[i].hostIP));
                    printf("Se actualizara su Unix Timestamp\n\n");
                    if (actualizarHostsExpiracion(ipPuerto, time(NULL)) < 0)
                        rutinaDeError("consulta hosts expiracion");
                }
            }
            else printf("Aun quedan %ld para enviar Crawler a %s\n\n",
                                tiempoMigracion - tiempoRestante, inet_ntoa(hosts[i].hostIP));
            i++;
        }        
        if (hosts != NULL)  free(hosts);
    }
    
    exit(EXIT_SUCCESS);
}

/*
Descripcion: Genera una tabla de hosts segun lo almacenado en el dir de expiracion de ldap
Ultima modificacion: Scheinkman, Mariano
Recibe: lista de hosts actuales, numero maximo de hosts
Devuelve: ok? 0: -1. lista de hosts actualizada, y numero maximo de hosts
*/
int consultarHostsExpiracion(webServerHosts **hosts, int *maxHosts)
{
    /*HACER*/

    return 0;
}

/*
Descripcion: Actualiza el dir de expiracion de ldap, con el tiempo enviado
Ultima modificacion: Scheinkman, Mariano
Recibe: nuevo uts
Devuelve: ok? 0: -1.
*/
int actualizarHostsExpiracion(time_t nuevoUts)
{
    /*HACER*/

    return 0;
}


int EnviarCrawlerCreate(in_addr_t nDireccion, in_port_t nPort)
{
    SOCKET sockWebServer;
    SOCKADDR_IN dirServidorWeb;
    char descriptorID[DESCRIPTORID_LEN];

    /*Se levanta conexion con el Web Server*/
    if ((sockWebServer = establecerConexionServidorWeb(nDireccion, nPort, &dirServidorWeb)) < 0)
        return -1;
    printf("Se establecio conexion con WebServer en %s.\n", inet_ntoa(dirServidorWeb.sin_addr));

    /*Se envia mensaje de instanciacion de un Crawler*/
    if (ircRequest_send(sockWebServer, NULL, 0, descriptorID, IRC_CRAWLER_CREATE) < 0)
    {
        close(sockWebServer);
        return -1;
    }
    printf("Crawler disparado a dirServidorWeb.sin_addr.\n\n", inet_ntoa(dirServidorWeb.sin_addr));

    close(sockWebServer);
    
    return 0;
}

SOCKET establecerConexionServidorWeb(in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *their_addr)
{
    SOCKET sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        rutinaDeError("socket");

    their_addr->sin_family = AF_INET;
    their_addr->sin_port = nPort;
    their_addr->sin_addr.s_addr = nDireccionIP;
    memset(&(their_addr->sin_zero),'\0',8);

    if (connect(sockfd, (struct sockaddr *)their_addr, sizeof(struct sockaddr)) == -1)
        rutinaDeError("connect");

    return sockfd;
}


void signalHandler(int sig)
{
    switch(sig)
    {
        case SIGUSR1:
            /*Señal recibida*/
            sigRecibida=1;
            printf("Señal recibida, sigRecibida = %d\n\n", sigRecibida);
            break;
    }
    if (signal(sig, signalHandler) == SIG_ERR)
        rutinaDeError("signal");
}

void rutinaDeError(char *error)
{
    fprintf(stderr, "Error al disparar Crawlers. ");
    perror(error);
    exit(EXIT_FAILURE);
}
