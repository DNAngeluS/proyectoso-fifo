#include "crawler-create.h"
#include "irc.h"
#include "mldap.h"

void rutinaDeError(char *error, int log);
void signalHandler(int sig);

int establecerConexionServidorWeb(in_addr_t nDireccion, in_port_t nPort, SOCKADDR_IN *dir);
int EnviarCrawler(in_addr_t nDireccion, in_port_t nPort, int *mode);

volatile sig_atomic_t sigRecibida = 0;
int log;
pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv)
{
    struct sigaction new_action, old_action;
    configuracion config;
    time_t actualTime;
    ldapObj ldap;
    char ipPuertoConocido[MAX_PATH];
    int modeConocido = 0x00;
    mode_t modeOpen = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    /*sleep(5);*/

    if (argc != 6)
        rutinaDeError("Argumentos invalidos", log);

    memset(&config, '\0', sizeof(configuracion));
    memset(ipPuertoConocido, '\0', sizeof(ipPuertoConocido));

    /*Inicializar estructura config con valores pasado por argv*/
    config.ipWebServer = atoi(argv[1]);
    config.puertoWebServer = atoi(argv[2]);
    config.tiempoMigracionCrawler = atoi(argv[3]);
    strcpy(config.ipPortLDAP, argv[4]);
    strcpy(config.claveLDAP, argv[5]);

     /*Inicializacion de los mutex*/
    if (pthread_mutex_init(&logMutex, NULL) < 0)
       rutinaDeError("Mutex init", log);

    /*Se crea el archivo log*/
    if ((log = open("log-crawler-create.txt", O_CREAT | O_TRUNC | O_WRONLY, modeOpen)) < 0)
        rutinaDeError("Crear archivo Log", log);
    WriteLog(log, "Crawler Create", getpid(), 1, "Inicio de ejecucion", "INFO");

    /*Establecer conexion LDAP*/
    WriteLog(log, "Crawler Create", getpid(), 1, "Se establecera conexion LDAP", "INFO");
    if(establecerConexionLDAP(&ldap, config) < 0)
      rutinaDeError("No se pudo establecer la conexion LDAP.", log);
    WriteLog(log, "Crawler Create", getpid(), 1, "Conexion establecida OK", "INFOFIN");

    /*Se asigna un handler a la señal, si no se bloqueo su captura antes*/
    if (signal(SIGUSR1, signalHandler) == SIG_ERR)
        rutinaDeError("signal", log);

    /*Se inicialian controladores para bloquear hasta recibir SIGUSR1*/
    sigemptyset (&new_action.sa_mask);
    sigaddset (&new_action.sa_mask, SIGUSR1);

    /*Se envia pedido de creacion de Crawler al primer WebServer conocido*/
    WriteLog(log, "Crawler Create", getpid(), 1, "Se enviara instanciacion de Crawler al WebServer conocido", "INFO");
    if (EnviarCrawler(config.ipWebServer, config.puertoWebServer, &modeConocido) < 0)
	{
            WriteLog(log, "Crawler Create", getpid(), 1, "Enviado primer Crawler", "ERROR");
            rutinaDeError("Enviando primer Crawler", log);
	}
	else
            WriteLog(log, "Crawler Create", getpid(), 1, "Enviado OK", "INFOFIN");

    /*Se actualiza el timestamp del host al que se le envio el Crawler*/
    sprintf(ipPuertoConocido, "%s:%d", inet_ntoa(*(struct in_addr *) &(config.ipWebServer)), ntohs(config.puertoWebServer));
    ldapActualizarHost(&ldap, ipPuertoConocido, time(NULL), ALTA);


    while (1)
    {
        int i = 0;
        int maxHosts = 0;
        webServerHosts *hosts = NULL;
        char text[100];

        sprintf(text, "--(hijo)Esperando SIGUSR1... pid(%d)--", getpid());
        WriteLog(log, "Crawler Create", getpid(), 1, text, "INFOFIN");
        
        /*Se espera por el SIGUSR1 para comenzar y de aqui en mas no se atendera mas*/
        sigprocmask (SIG_BLOCK, &new_action.sa_mask, &old_action.sa_mask);
        while (!sigRecibida)
            sigsuspend (&old_action.sa_mask);
        sigprocmask (SIG_UNBLOCK, &new_action.sa_mask, &old_action.sa_mask);

        /*Se devuelve el valor a la variable, para esperar una nueva señal*/
        sigRecibida=0;

        /*Se consulta el dir de expiracion y obtiene la tabla actualizada*/
        WriteLog(log, "Crawler Create", getpid(), 1, "Se obtendran los hosts del directorio de hosts", "INFO");
        if (ldapObtenerHosts(&ldap, &hosts, &maxHosts) < 0)
        {
            WriteLog(log, "Crawler Create", getpid(), 1, "Error", "ERROR");
            continue;
        }
        WriteLog(log, "Crawler Create", getpid(), 1, "Obtenidos OK", "INFOFIN");

        WriteLog(log, "Crawler Create", getpid(), 1, "Se dispararan Crawlers a hosts desactualizados", "INFOFIN");
      
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
                sprintf(text, "Se enviara instanciacion de Crawler a %s:%d", inet_ntoa(*(struct in_addr *) &(hosts[i].hostIP)), ntohs(hosts[i].hostPort));
                WriteLog(log, "Crawler Create", getpid(), 1, text, "INFO");
                if (EnviarCrawler(hosts[i].hostIP, hosts[i].hostPort, &mode) < 0)
                    WriteLog(log, "Crawler Create", getpid(), 1, "Error al enviar instanciacion", "ERROR");
                else
                {
                    WriteLog(log, "Crawler Create", getpid(), 1, "Instanciacion enviada OK", "INFOFIN");

                    /*Si pudo migrar*/
                    if (mode == IRC_CRAWLER_OK)
                    {
                            char ipPuerto[MAX_PATH];

                            WriteLog(log, "Crawler Create", getpid(), 1, "Crawler a migrado satisfactoriamente", "INFOFIN");
                            sprintf(ipPuerto, "%s:%d", inet_ntoa(*(struct in_addr *) &(hosts[i].hostIP)), ntohs(hosts[i].hostPort));

                            /*Se actualiza el timestamp del host al que se le envio el Crawler*/
                            WriteLog(log, "Crawler Create", getpid(), 1, "Se actualizara Unix Timestamp del host", "INFO");
                            if (ldapActualizarHost(&ldap, ipPuerto, time(NULL), MODIFICACION) < 0)
                                WriteLog(log, "Crawler Create", getpid(), 1, "Error al actualizar Unix Timestamp", "ERROR");
                            else
                                WriteLog(log, "Crawler Create", getpid(), 1, "Unix Timestamp actualizado OK", "INFOFIN");
                    }
                    else if (mode == IRC_CRAWLER_FAIL)
                    {
                        WriteLog(log, "Crawler Create", getpid(), 1, "Crawler a sido rechazado", "INFOFIN");
                    }
                }
            }
            else
            {
                sprintf(text, "Aun quedan %ld segundos para enviar Crawler a %s",
                                        config.tiempoMigracionCrawler - tiempoRestante,
                                        inet_ntoa(*(struct in_addr *) &(hosts[i].hostIP)));
                WriteLog(log, "Crawler Create", getpid(), 1, text, "INFOFIN");
            }
            i++;
        }        
        
        if (hosts != NULL)
            free(hosts);
        else
        {
            WriteLog(log, "Crawler Create", getpid(), 1, "No se encontraron hosts en el directorio de expiracion de hosts", "INFOFIN");
            putchar('\n');
        }
    }

    /*Finalizo el mutex*/
    pthread_mutex_destroy(&logMutex);
    close(log);

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

	memset(buf, '\0', sizeof(buf));

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
        return -1;

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
        rutinaDeError("signal", log);
}

void rutinaDeError(char* error, int log)
{
    printf("\r\nError: %s\r\n", error);
    printf("Codigo de Error: %d\r\n\r\n", errno);
    perror(error);

    /*Mutua exclusion*/
    WriteLog(log, "Front-end", getpid(), 1, error, "ERROR");

    exit(EXIT_FAILURE);
}
