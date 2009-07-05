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

void rutinaDeError(char* error);
void signalHandler(int sig);
SOCKET establecerConexionEscucha(in_addr_t nDireccionIP, in_port_t nPort);

int atenderCrawler(SOCKET sockCrawler, ldapObj ldap);
int atenderAltaURL(ldapObj *ldap, crawler_URL *entrada, int mode);
int atenderModificarURL(ldapObj *ldap, crawler_URL *entrada, int mode);

configuracion config;
volatile sig_atomic_t sigRecibida=0;
jmp_buf entorno;
pid_t childID = 0;

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
    
    /*Lectura de Archivo de Configuracion*/
    printf("Se leera archivo de configuracion. ");
    if (leerArchivoConfiguracion(&config) != 0)
       rutinaDeError("Lectura Archivo de configuracion");
    printf("Leido OK.\n");

    /*Establecer conexion LDAP*/
	printf("Se establecera conexion con ldap. ");
    if (establecerConexionLDAP(&ldap, config) < 0)
      rutinaDeError("Conexion ldap");
    printf("Conexion establecida OK.\n");

    /*Se establece conexion a puerto de escucha*/
	printf("Se establecera conexion de escucha. ");
    if ((sockWebStore = establecerConexionEscucha(INADDR_ANY, config.puertoL)) == INVALID_SOCKET)
        rutinaDeError("Socket invalido");
	printf("Conexion establecida OK.\n");

    saltoDeInterrupcion += setjmp(entorno);
    if (saltoDeInterrupcion > 0)
    {
        printf("Se trato de interrumpir el proceso por %d vez. Se retorna a un punto seguro.\n\n", saltoDeInterrupcion);
        sigRecibida = 0;
    }

	/*Se asigna un handler a las señales, si no se bloqueo su captura antes*/
    if (signal(SIGCHLD, signalHandler) == SIG_ERR)
        rutinaDeError("signal");
    if (signal(SIGUSR1, signalHandler) == SIG_ERR)
        rutinaDeError("signal");
    if (signal(SIGSEGV, signalHandler) == SIG_ERR)
        rutinaDeError("signal");

    /*Se inicialian controladores para bloquear hasta recibir SIGUSR1*/
    sigemptyset (&new_action.sa_mask);
    sigaddset (&new_action.sa_mask, SIGUSR1);

    printf("--(padre)Esperando SIGUSR1... pid(%d)--\n", getpid());

    sigprocmask (SIG_BLOCK, &new_action.sa_mask, &old_action.sa_mask);
    /*Se espera por el SIGUSR1 para comenzar y de aqui en mas no se atendera mas*/
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

		printf("Se creara el proceso Crawler-Create. ");
        if ((childID = fork()) < 0)
        {
            rutinaDeError("Creacion de proceso crawler-create");
        }
        else if (childID == 0)
        /*Este es el hijo*/
        {            
            char *argv[] = {"crawler-create", ip, puerto,
                            tiempoNuevaConsulta, config.ipPortLDAP, config.claveLDAP, NULL};
            execv("./crawler-create", argv);

            /*Si ejectua esto fue porque fallo execv*/
            rutinaDeError("execv");
        }
        else if (childID > 0)
        /*Este es el padre*/
        {
            printf("Proceso creado OK.\n", childID);
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
            printf("Esperando datos de Crawlers...\n");
        }

        rc = select(fdMax+1, &fdLectura, NULL, NULL, &timeout);

        if (rc < 0)
        /*Error en select()*/
            rutinaDeError("select");

        if (rc == 0)
        /*Select timeout*/
        {
            if (childID != 0)
                kill(childID, SIGUSR1);
            printf("Señal SIGURS1 enviada a Crawler-Create.\n");
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
                            perror("No se pudo conectar Crawler");
                            continue;
                        }

                        /*Agrega cliente y actualiza max*/
                        FD_SET (sockCrawler, &fdMaestro);
                        if (sockCrawler > fdMax)
                                fdMax = sockCrawler;

                        printf("Crawler conectado desde %s.\n", inet_ntoa(dirCrawler.sin_addr));
                    }
                    else
                    /*Crawler detectado -> esta enviando consultas*/
                    {
                        /*Atiende al Crawler*/
						printf("Se atendera nuevo Crawler.\n");
                        if (atenderCrawler(cli, ldap) < 0)
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
    printf("Se recibira un paquete del Crawler. ");
    if (ircPaquete_recv(sockCrawler, &paquete, descID, &mode) < 0)
    {
        printf("Error.\n");
        return -1;
    }
	else
		printf("Recibido Ok.\n");

    if (paquete.palabras == NULL)
	{
        printf("No se detectaron palabras para ese URL. Sera descartado.\n\n");
		return 0;
	}

	/*Se modificara o dara de alta el URL entrante*/
    if (mode == IRC_CRAWLER_ALTA_HTML || mode == IRC_CRAWLER_ALTA_ARCHIVOS)
       atenderAltaURL(&ldap, &paquete, mode);
    else if (mode == IRC_CRAWLER_MODIFICACION_HTML || mode == IRC_CRAWLER_MODIFICACION_ARCHIVOS)
       atenderModificarURL(&ldap, &paquete, mode);

    else
    {
        printf("Inconcistencia en Payload Descriptor.\n");
        return -1;
    }

    printf("Crawler a sido atendido satisfactoriamente.\n\n");
    free(paquete.palabras);
    
    return 0;
}

int atenderAltaURL(ldapObj *ldap, crawler_URL *entrada, int mode)
{
    char ipPuerto[MAX_PATH];
    char *ptr, *lim;;
    int existeHost;

    /*Obtiene la clave IP:PUERTO del URL*/
    memset(ipPuerto, '\0', sizeof(ipPuerto));
    ptr = strchr(entrada->URL, '/');
    ptr +=2 ;
    lim = strchr(ptr, ':');
    strncpy(ipPuerto, ptr, lim - ptr);
    strcat(ipPuerto, ":");
    strcat(ipPuerto, config.puertoWebServer);

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
                rutinaDeError("signal");
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
                rutinaDeError("signal");
            raise(sig);
            break;

    }
    if (!noSig)
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
    kill(childID, SIGINT);
    perror(error);
    exit(EXIT_FAILURE);
}
