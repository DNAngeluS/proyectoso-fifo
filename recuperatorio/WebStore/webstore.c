/* 
 * File:   webstore.c
 * Author: marianoyfer
 *
 * Created on 15 de mayo de 2009, 11:46
 */
#include "webstore.h"
#include "config.h"
#include "irc.h"
#include "mldap.h"
#include "log.h"

void rutinaDeError(char* error, int log);
void signalHandler(int sig);
SOCKET establecerConexionEscucha(in_addr_t nDireccionIP, in_port_t nPort);

int atenderCrawler(SOCKET sockCrawler, ldapObj ldap);
int atenderAltaURL(ldapObj *ldap, crawler_URL *entrada, int mode);
int atenderModificarURL(ldapObj *ldap, crawler_URL *entrada, int mode);

configuracion config;
volatile sig_atomic_t sigRecibida=0;
jmp_buf entorno;
pid_t childID = 0;
int log;
pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * 
 */
int main()
{
    SOCKET sockWebStore;
    ldapObj ldap;
    struct sigaction new_action, old_action;
    fd_set fdMaestro;
    fd_set fdLectura;
    struct timeval timeout;
    int fdMax, cli;
    int mensajeEsperando = 0;
    int saltoDeInterrupcion = 0;
    char text[100];
    mode_t modeOpen = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    /*Inicializacion de los mutex*/
    if (pthread_mutex_init(&logMutex, NULL) < 0)
       rutinaDeError("Mutex init", log);

    /*Se crea el archivo log*/
    if ((log = open("log.txt", O_CREAT | O_TRUNC | O_WRONLY, modeOpen)) < 0)
        rutinaDeError("Crear archivo Log", log);
    WriteLog(log, "Web Store", getpid(), 1, "Inicio de ejecucion", "INFO");

    /*Lectura de Archivo de Configuracion*/
    WriteLog(log, "Web Store", getpid(), 1, "Se leera archivo de configuracion", "INFO");
    if (leerArchivoConfiguracion(&config) != 0)
       rutinaDeError("Lectura Archivo de configuracion", log);
    WriteLog(log, "Web Store", getpid(), 1, "Leido OK", "INFOFIN");

    /*Establecer conexion LDAP*/
    WriteLog(log, "Web Store", getpid(), 1, "Se establecera LDAP", "INFO");
    if (establecerConexionLDAP(&ldap, config) < 0)
      rutinaDeError("Conexion ldap", log);
    WriteLog(log, "Web Store", getpid(), 1, "Conexion establecida OK", "INFOFIN");

    /*Se establece conexion a puerto de escucha*/
    WriteLog(log, "Web Store", getpid(), 1, "Se establecera conexion de escucha", "INFO");
    if ((sockWebStore = establecerConexionEscucha(INADDR_ANY, config.puertoL)) == INVALID_SOCKET)
        rutinaDeError("Socket invalido", log);
    WriteLog(log, "Web Store", getpid(), 1, "Conexion establecida OK", "INFOFIN");

    saltoDeInterrupcion += setjmp(entorno);
    if (saltoDeInterrupcion > 0)
    {
        sprintf(text, "Se trato de interrumpir el proceso por %d vez. Se retorna a un punto seguro", saltoDeInterrupcion);
        WriteLog(log, "Web Store", getpid(), 1, text, "INFOFIN");
        sigRecibida = 0;
    }

	/*Se asigna un handler a las señales, si no se bloqueo su captura antes*/
    if (signal(SIGCHLD, signalHandler) == SIG_ERR)
        rutinaDeError("signal sigchld", log);
    if (signal(SIGUSR1, signalHandler) == SIG_ERR)
        rutinaDeError("signal sigurs1", log);
    if (signal(SIGSEGV, signalHandler) == SIG_ERR)
        rutinaDeError("signal sigsegv", log);

    /*Se inicialian controladores para bloquear hasta recibir SIGUSR1*/
    sigemptyset (&new_action.sa_mask);
    sigaddset (&new_action.sa_mask, SIGUSR1);

    sprintf(text, "--(padre)Esperando SIGUSR1... pid(%d)--", getpid());
    WriteLog(log, "Web Store", getpid(), 1, text, "INFOFIN");

     /*Se espera por el SIGUSR1 para comenzar y de aqui en mas no se atendera mas*/
    sigprocmask (SIG_BLOCK, &new_action.sa_mask, &old_action.sa_mask);
    while (!sigRecibida)
        sigsuspend (&old_action.sa_mask);
    sigprocmask (SIG_UNBLOCK, &new_action.sa_mask, &old_action.sa_mask);

    if (sigRecibida)
    {
        char puerto[20], ip[25], tiempoNuevaConsulta[20];

        memset(puerto, '\0', 20);
        memset(ip, '\0', 25);
        memset(tiempoNuevaConsulta, '\0', 20);

        /*Se cargan los datos para enviar al proceso Hijo como parametro*/
        sprintf(ip, "%d", config.ipWebServer);
        sprintf(puerto, "%d", config.puertoWebServer);
        sprintf(tiempoNuevaConsulta, "%d", config.tiempoNuevaConsulta);

        WriteLog(log, "Web Store", getpid(), 1, "Se creara el proceso crawler-create", "INFO");
        if ((childID = fork()) < 0)
        {
            rutinaDeError("Creacion de proceso crawler-create", log);
        }
        else if (childID == 0)
        /*Este es el hijo*/
        {            
            char *argv[] = {"crawler-create", ip, puerto,
                                        tiempoNuevaConsulta, config.ipPortLDAP,
                                        config.claveLDAP, NULL};
            execv("./crawler-create", argv);

            /*Si ejectua esto fue porque fallo execv*/
            rutinaDeError("execv", log);
        }
        else if (childID > 0)
        /*Este es el padre*/
        {
            WriteLog(log, "Web Store", getpid(), 1, "Proceso creado OK", "INFOFIN");
        }
    }

    /*Se inicializan datos para el select()*/
    FD_ZERO (&fdMaestro);
    FD_SET(sockWebStore, &fdMaestro);
    fdMax = sockWebStore;
    signal(SIGINT, signalHandler);

    
    while (1)
    {
        int rc, desc_ready;

        /*Se establece un tiempo de timeout para select()*/
        timeout.tv_sec = config.tiempoNuevaConsulta;
        timeout.tv_usec = 0;

        FD_ZERO(&fdLectura);
        memcpy(&fdLectura, &fdMaestro, sizeof(fdMaestro));

        if (!mensajeEsperando)
        {
            mensajeEsperando = 1;
           WriteLog(log, "Web Store", getpid(), 1, "Esperando datos de Crawlers", "INFOFIN");
        }

        rc = select(fdMax+1, &fdLectura, NULL, NULL, &timeout);

        if (rc < 0)
        /*Error en select()*/
            rutinaDeError("select", log);

        if (rc == 0)
        /*Select timeout*/
        {
            if (childID != 0)
                kill(childID, SIGUSR1);
            WriteLog(log, "Query Manager", getpid(), 1, "Señal SIGURS1 enviada a crawler-create", "INFOFIN");
            putchar('\n');
            mensajeEsperando = 0;
            
            /*pause();*/
        }

        if (rc > 0)
        /*Se detectaron sockets con actividad*/
        {
            desc_ready = rc;
            mensajeEsperando = 0;
            
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
                            WriteLog(log, "Web Store", getpid(), 1, "No se pudo conectar Crawler", "ERROR");
                            continue;
                        }

                        /*Agrega cliente y actualiza max*/
                        FD_SET (sockCrawler, &fdMaestro);
                        if (sockCrawler > fdMax)
                                fdMax = sockCrawler;

                        sprintf(text, "Crawler conectado desde %s.\n", inet_ntoa(dirCrawler.sin_addr));
                        WriteLog(log, "Web Store", getpid(), 1, text, "INFOFIN");
                    }
                    else
                    /*Crawler detectado -> esta enviando consultas*/
                    {
                        /*Atiende al Crawler*/
                        WriteLog(log, "Web Store", getpid(), 1, "Se atendera nuevo Crawler", "INFO");
                        if (atenderCrawler(cli, ldap) < 0)
                            WriteLog(log, "Web Store", getpid(), 1, "Hubo un error al atender Crawler", "ERROR");
                        else
                            WriteLog(log, "Web Store", getpid(), 1, "Atencion del Crawler finalizada satisfactoriamente", "INFOFIN");

                        WriteLog(log, "Web Store", getpid(), 1, "Se cierra conexion con Crawler", "INFOFIN");
                        putchar('\n');

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

    /*Mata al hijo*/
    kill(childID, SIGINT);

    /*Cierra conexiones de Sockets*/
    close(sockWebStore);
    for (cli = 0; cli < fdMax+1; ++cli)
        close(cli);

    /*Cierra conexiones LDAP*/
    ldap.sessionOp->endSession(ldap.session);

    /*Libero los objetos de operaciones*/
    freeLDAPSession(ldap.session);
    freeLDAPContext(ldap.context);
    freeLDAPContextOperations(ldap.ctxOp);
    freeLDAPSessionOperations(ldap.sessionOp);

    /*Finalizo el mutex*/
    pthread_mutex_destroy(&logMutex);
    close(log);
    
    return (EXIT_SUCCESS);
}

int atenderCrawler(SOCKET sockCrawler, ldapObj ldap)
{
    crawler_URL paquete;
    char descID[DESCRIPTORID_LEN];
    unsigned long paqueteLen = 0;
    int mode=0x00;
    
    memset(&paquete, '\0', sizeof(paquete));

	/*Se recibira el paquete URL del Crawler*/
    WriteLog(log, "Web Store", getpid(), 1, "Se recibira un paquete del Crawler", "INFO");
    sleep(1);
    if (ircPaquete_recv(sockCrawler, &paquete, descID, &mode) < 0)
    {
        WriteLog(log, "Web Store", getpid(), 1, "Error", "ERROR");
        return -1;
    }
	else
            WriteLog(log, "Web Store", getpid(), 1, "Recibido OK", "INFOFIN");

    if (paquete.palabras == NULL)
    {
        WriteLog(log, "Web Store", getpid(), 1, "No se detectaron palabras para ese URL. Sera descartado", "ERROR");
        putchar('\n');  
        return 0;
    }

	/*Se modificara o dara de alta el URL entrante*/
    if (mode == IRC_CRAWLER_ALTA_HTML || mode == IRC_CRAWLER_ALTA_ARCHIVOS)
       atenderAltaURL(&ldap, &paquete, mode);
    else if (mode == IRC_CRAWLER_MODIFICACION_HTML || mode == IRC_CRAWLER_MODIFICACION_ARCHIVOS)
       atenderModificarURL(&ldap, &paquete, mode);

    else
    {
        WriteLog(log, "Web Store", getpid(), 1, "Inconcistencia en payload descriptor", "ERROR");
        return -1;
    }

    WriteLog(log, "Web Store", getpid(), 1, "Crawler a sido atendido satisfactoriamente", "INFOFIN");
    putchar('\n');
    free(paquete.palabras);
    
    return 0;
}

int atenderAltaURL(ldapObj *ldap, crawler_URL *entrada, int mode)
{
    char ipPuerto[MAX_PATH];
    char aux[MAX_PATH];
    char *ptr, *lim;;
    int existeHost;

    /*Obtiene la clave IP:PUERTO del URL*/
    memset(ipPuerto, '\0', sizeof(ipPuerto));
    ptr = strchr(entrada->URL, '/');
    ptr +=2 ;
    lim = strchr(ptr, ':');
    strncpy(aux, ptr, lim - ptr);
    sprintf(ipPuerto, "%s:%d", aux, ntohs(config.puertoWebServer));

    /*Si el host no existe aun lo agrega con un nuevo Unix Timestamp*/
    existeHost = ldapComprobarExistencia(ldap, ipPuerto, IRC_CRAWLER_HOST);
    if (!existeHost)
        ldapActualizarHost(ldap, ipPuerto, time(NULL), ALTA);

    if (ldapAltaURL(ldap, entrada, mode) < 0)
        return -1;

    return 0;
}

int atenderModificarURL(ldapObj *ldap, crawler_URL *entrada, int mode)
{

    if (ldapModificarURL(ldap, entrada, mode) < 0)
        return -1;

    return 0;
}

void signalHandler(int sig)
{
    int noSig=0;
    switch(sig)
    {
        case SIGUSR1:
            noSig = 1;
            sigRecibida = 1;
            if (signal(sig, SIG_IGN) == SIG_ERR)
                rutinaDeError("signal sig ignore->sigusr1", log);
            break;
        case SIGCHLD:
            while (waitpid(-1, NULL, WNOHANG) > 0);
            longjmp(entorno, 1);
            break;
        case SIGINT:
            /*Hubo una interrupcion. Se retorno al while(1) principal*/
            longjmp(entorno, 1);
            break;
        case SIGSEGV:
            noSig = 1;
            kill(childID, SIGKILL);
            if (signal(sig, SIG_DFL) == SIG_ERR)
                rutinaDeError("signal sigg default-> sigsegv", log);
            raise(sig);
            break;

    }
    if (!noSig)
        if (signal(sig, signalHandler) == SIG_ERR)
        rutinaDeError("signal signalHandler", log);
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
            rutinaDeError("Setsockopt", log);

        /*if (ioctlsocket(sockfd, FIONBIO, &NonBlock) == SOCKET_ERROR)
        {
            rutinaDeError("ioctlsocket", log);
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
                rutinaDeError("Listen", log);
            else
            return sockfd;
        }
        else
            rutinaDeError("Bind", log);
    }

    return INVALID_SOCKET;
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
