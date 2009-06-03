#include "crawler-create.h"
#include "irc.h"
#include "mldap.h"

void rutinaDeError(char *error);
void signalHandler(int sig);

int establecerConexionServidorWeb(in_addr_t nDireccion, in_port_t nPort, SOCKADDR_IN *dir);
int EnviarCrawlerCreate(in_addr_t nDireccion, in_port_t nPort);

volatile sig_atomic_t sigRecibida = 0;

int main(int argc, char **argv)
{
    struct sigaction old_action;
    configuracion config;
    time_t actualTime;
    ldapObj ldap;

    /*sleep(10);*/

    if (argc != 6)
        rutinaDeError("Argumentos invalidos");

    memset(&config, '\0', sizeof(configuracion));

    /*Inicializar estructura config con valores pasado por argv*/
    config.ipWebServer = atoi(argv[1]);
    config.puertoWebServer = atoi(argv[2]);
    config.tiempoMigracionCrawler = atoi(argv[3]);
    strcpy(config.ipPortLDAP, argv[4]);
    strcpy(config.claveLDAP, argv[5]);

    /*Establecer conexion LDAP*/
    if(establecerConexionLDAP(&ldap, config) < 0)
      rutinaDeError("No se pudo establecer la conexion LDAP.");
    printf("Conexion establecida con LDAP\n");

    /*Se asigna un handler a la señal, si no se bloqueo su captura antes*/
    if (signal(SIGUSR1, signalHandler) == SIG_ERR)
        rutinaDeError("signal");

    /*Se envia pedido de creacion de Crawler al primer WebServer conocido*/
    if (EnviarCrawlerCreate(config.ipWebServer, config.puertoWebServer) < 0)
        rutinaDeError("Enviando primer Crawler");

    while (1)
    {
        int i = 0;
        int maxHosts = 0;
        webServerHosts *hosts = NULL;

        printf("Esperando SIGUSR1... pid(%d)\n", getpid());

        /*Se espera por el SIGUSR1 para comenzar*/
        while (!sigRecibida)
            sigsuspend (&old_action.sa_mask);

        /*Se devuelve el valor a la variable, para esperar una nueva señal*/
        sigRecibida=0;

        /*Se consulta el dir de expiracion y obtiene la tabla actualizada*/
        if (ldapObtenerHosts(&ldap, &hosts, &maxHosts) < 0)
            rutinaDeError("consulta hosts expiracion");

        printf("Se consulto directorio de hosts satisfactoriamente.\n");
        printf("Se dispararan Crawlers a hosts desactualizados\n");
        
        while (i < maxHosts)
        /*Para cada hosts en la tabla*/
        {
            long tiempoRestante;
            actualTime = time(NULL);
            tiempoRestante = (actualTime - hosts[i].uts);

            if (tiempoRestante >= config.tiempoMigracionCrawler)
            /*Si el tiempo que paso desde la ultima migracion es mayor al configurado*/
            {
                /*Envia pedido de creacion de Crawler*/
                if (EnviarCrawlerCreate(hosts[i].hostIP, hosts[i].hostPort) < 0)
                {
                    fprintf(stderr, "Error: Instanciacion de un Crawler a %s:%d\n",
                            inet_ntoa(*(struct in_addr *) (hosts[i].hostIP)), ntohs(hosts[i].hostPort));
                    continue;
                }
                else
                {
                    char ipPuerto[MAX_PATH];

                    sprintf(ipPuerto, "%s:%d", inet_ntoa(*(struct in_addr *) (hosts[i].hostIP)), ntohs(hosts[i].hostPort));

                    printf("Se envio peticion de Crawler a %s.\n", inet_ntoa(*(struct in_addr *) (hosts[i].hostIP)));
                    printf("Se actualizara su Unix Timestamp\n\n");

                    /*Se actualiza el timestamp del host al que se le envio el Crawler*/
                    if (ldapActualizarHost(&ldap, ipPuerto, time(NULL), MODIFICACION) < 0)
                        rutinaDeError("consulta hosts expiracion");
                }
            }
            else printf("Aun quedan %ld segundos para enviar Crawler a %s\n\n",
                                        config.tiempoMigracionCrawler - tiempoRestante,
                                        inet_ntoa(*(struct in_addr *) (hosts[i].hostIP)));
            i++;
        }        
        
        if (hosts != NULL)
            free(hosts);
        else
            printf("No se encontraron hosts en el Directorio de Expiracion de Hosts\n\n");
    }

    kill(SIGCHLD, getppid());
    exit(EXIT_SUCCESS);
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
    printf("Crawler disparado a %s.\n\n", inet_ntoa(dirServidorWeb.sin_addr));

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

    /*if (connect(sockfd, (struct sockaddr *)their_addr, sizeof(struct sockaddr)) == -1)
        rutinaDeError("connect");*/

    while ( connect (sockfd, (struct sockaddr *)their_addr, sizeof(struct sockaddr)) == -1 && errno != EISCONN )
        if ( errno != EINTR )
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
    kill(SIGCHLD, getppid());
    exit(EXIT_FAILURE);
}
