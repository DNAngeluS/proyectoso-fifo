#include "crawler-create.h"
#include "irc.h"

void rutinaDeError(char *error);
void signalHandler(int sig);

int establecerConexionServidorWeb(in_addr_t nDireccion, in_port_t nPort, SOCKADDR_IN *dir);
int EnviarCrawlerCreate(in_addr_t nDireccion, in_port_t nPort);


int sigRecibida = 0;

int main(int argc, char **argv)
{
    in_addr_t ipWebServer;
    in_port_t puertoWebServer;
    unsigned int tiempoMigracion;
    time_t actualTime;

    if (argc != 3)
        rutinaDeError("Argumentos invalidos");

    /*Inicializa parametros de Web Server conocido y tiempo de nueva Migracion*/
    ipWebServer = atoi(argv[1]);
    puertoWebServer = atoi(argv[2]);
    tiempoMigracion = atoi(argv[3]);

    /*Se asigna un handler a la señal*/
    if (signal(SIGUSR1, signalHandler) == SIG_ERR)
        rutinaDeError("signal");

    /*Se envia pedido de creacion de Crawler al primer WebServer conocido*/
    if (EnviarCrawlerCreate(ipWebServer, puertoWebServer) < 0)
        rutinaDeError("Enviando primer Crawler");

    while (1)
    {
        if (sigRecibida)
        /*Si se recibio la señal para iniciar la migracion*/
        {
            int maxHosts, i;
            webServerHosts *hosts = NULL;

            memset(hosts,'\0',sizeof(webServerHosts));
            maxHosts = 0;

            /*Se consulta el dir de expiracion y obtiene la tabla actualizada*/
            if (consultarHostsExpiracion(&hosts, &maxHosts) < 0)
                rutinaDeError("consulta hosts expiracion");
   
            for (i=0; i<maxHosts; i++)
            /*Para cada hosts en la tabla*/
            {
                actualTime = time(NULL);
                if ((actualTime - hosts[i].uts) >= tiempoMigracion)
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
                        if (actualizarHostsExpiracion(time(NULL)) < 0)
                            rutinaDeError("consulta hosts expiracion");
                    }
                }
            }
            free(hosts);
            
            /*Vuelve a establecer la señal como no recibida*/
            sigRecibida = 0;
        }
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
    if (ircRequest_send(sockWebServer, NULL, sizeof(NULL), descriptorID, IRC_CRAWLER_CREATE) < 0)
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
            sigRecibida = 1;
            break;
    }
    if (signal(sig, signalHandler) == SIG_ERR)
        rutinaDeError("signal");
}

void rutinaDeError(char *error)
{
    fprintf(stderr, "Error al disparar Crawlers. %s", error);
    exit(EXIT_FAILURE);
}
