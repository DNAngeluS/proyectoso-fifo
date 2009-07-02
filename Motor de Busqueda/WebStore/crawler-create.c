#include "crawler-create.h"
#include "irc.h"
#include "mldap.h"

void rutinaDeError(char *error);
void signalHandler(int sig);

int establecerConexionServidorWeb(in_addr_t nDireccion, in_port_t nPort, SOCKADDR_IN *dir);
int EnviarCrawler(in_addr_t nDireccion, in_port_t nPort);

volatile sig_atomic_t sigRecibida = 0;

int main(int argc, char **argv)
{
    struct sigaction new_action, old_action;
    configuracion config;
    time_t actualTime;
    ldapObj ldap;
    char ipPuertoConocido[MAX_PATH];

    sleep(5);

    if (argc != 6)
        rutinaDeError("Argumentos invalidos");

    memset(&config, '\0', sizeof(configuracion));
    memset(ipPuertoConocido, '\0', sizeof(ipPuertoConocido));

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

    /*Se inicialian controladores para bloquear hasta recibir SIGUSR1*/
    sigemptyset (&new_action.sa_mask);
    sigaddset (&new_action.sa_mask, SIGUSR1);

    /*Se envia pedido de creacion de Crawler al primer WebServer conocido*/
	printf("Se enviara instanciacion de Crawler al Web Server Conocido. ");
    if (EnviarCrawler(config.ipWebServer, config.puertoWebServer) < 0)
	{
		printf("Error: la aplicacion no puede seguir.\n");
        rutinaDeError("Enviando primer Crawler");
	}
	else
		printf("Enviado OK.\n");

    /*Se actualiza el timestamp del host al que se le envio el Crawler*/
    sprintf(ipPuertoConocido, "%s:%d", inet_ntoa(*(struct in_addr *) &(config.ipWebServer)), ntohs(config.puertoWebServer));
    ldapActualizarHost(&ldap, ipPuertoConocido, time(NULL), ALTA);


    while (1)
    {
        int i = 0;
        int maxHosts = 0;
        webServerHosts *hosts = NULL;

        printf("--(hijo)Esperando SIGUSR1... pid(%d)--\n", getpid());

        sigprocmask (SIG_BLOCK, &new_action.sa_mask, &old_action.sa_mask);
        /*Se espera por el SIGUSR1 para comenzar y de aqui en mas no se atendera mas*/
        while (!sigRecibida)
            sigsuspend (&old_action.sa_mask);
        sigprocmask (SIG_UNBLOCK, &new_action.sa_mask, &old_action.sa_mask);

        /*Se devuelve el valor a la variable, para esperar una nueva señal*/
        sigRecibida=0;

        /*Se consulta el dir de expiracion y obtiene la tabla actualizada*/
		printf("Se obtendran los hosts del Directorio de Hosts. ");
        if (ldapObtenerHosts(&ldap, &hosts, &maxHosts) < 0)
		{
			printf("Error.\n");
            continue;
		}
        printf("Ok.\n");

        printf("Se dispararan Crawlers a hosts desactualizados\n");
      
        while (i < maxHosts)
        /*Para cada hosts en la tabla*/
        {
			int mode = 0x00;
            long tiempoRestante;
            actualTime = time(NULL);
            tiempoRestante = (actualTime - hosts[i].uts);

            if (tiempoRestante >= config.tiempoMigracionCrawler)
            /*Si el tiempo que paso desde la ultima migracion es mayor al configurado*/
            {
                /*Envia pedido de creacion de Crawler*/
				printf("Se enviara instanciacion de Crawler a %s:%d. ", inet_ntoa(*(struct in_addr *) &(hosts[i].hostIP)), ntohs(hosts[i].hostPort)););
                if (EnviarCrawler(hosts[i].hostIP, hosts[i].hostPort, &mode) < 0)
                    printf("Error.\n");
                else
                {
					printf("Enviado OK. ");

					/*Si pudo migrar*/
					if (mode == IRC_CRAWLER_OK)
					{
						char ipPuerto[MAX_PATH];

						printf("Crawler a migrado safisfactoriamente.\n");
						sprintf(ipPuerto, "%s:%d", inet_ntoa(*(struct in_addr *) &(hosts[i].hostIP)), ntohs(hosts[i].hostPort));
						
						/*Se actualiza el timestamp del host al que se le envio el Crawler*/
						printf("Se actualizara su Unix Timestamp. ");
						if (ldapActualizarHost(&ldap, ipPuerto, time(NULL), MODIFICACION) < 0)
							printf("Error.\n");
						else
							printf("Actualizado OK.\n");
					}
					else if (mode == IRC_CRAWLER_FAIL)
					{
						printf("Crawler a sido rechazado.\n");
					}
                }
            }
            else printf("Aun quedan %ld segundos para enviar Crawler a %s\n",
                                        config.tiempoMigracionCrawler - tiempoRestante,
                                        inet_ntoa(*(struct in_addr *) &(hosts[i].hostIP)));
            i++;
        }        
        
        if (hosts != NULL)
            free(hosts);
        else
            printf("No se encontraron hosts en el Directorio de Expiracion de Hosts\n\n");
    }

    kill(getppid(), SIGCHLD);
    exit(EXIT_SUCCESS);
}


int EnviarCrawler(in_addr_t nDireccion, in_port_t nPort, int *mode)
{
    SOCKET sockWebServer;
    SOCKADDR_IN dirServidorWeb;
    char descriptorID[DESCRIPTORID_LEN];
    char buf[BUF_SIZE];
    *mode = 0x00;

    /*Se levanta conexion con el Web Server*/
    if ((sockWebServer = establecerConexionServidorWeb(nDireccion, nPort, &dirServidorWeb)) < 0)
        return -1;

    /*Se envia mensaje de instanciacion de un Crawler*/
    if (ircRequest_send(sockWebServer, NULL, 0, descriptorID, IRC_CRAWLER_CONNECT) < 0)
    {
        close(sockWebServer);
        return -1;
    }

    if (ircRequest_recv (sockWebServer, buf, descriptorID, mode) < 0)
	{
		close(sockWebServer);
		return -1;
	}
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
        {
            close(sockfd);
            perror("connect");
            return -1;
        }
    
    return sockfd;
}


void signalHandler(int sig)
{
    switch(sig)
    {
        case SIGUSR1:
            /*Señal recibida*/
            sigRecibida=1;
            break;
    }
    if (signal(sig, signalHandler) == SIG_ERR)
        rutinaDeError("signal");
}

void rutinaDeError(char *error)
{
    fprintf(stderr, "Error al disparar Crawlers. ");
    perror(error);
    kill(getppid(), SIGCHLD);
    exit(EXIT_FAILURE);
}
