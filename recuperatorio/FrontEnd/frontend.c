/* 
 * File:   frontend.c
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 18:56
 */

#include "frontend.h"
#include "config.h"
#include "irc.h"
#include "http.h"
#include "log.h"

void rutinaDeError(char *string, int log);
void itoa_SearchType(int searchType, char *tipo);

SOCKET establecerConexionEscucha    (in_addr_t nDireccionIP, in_port_t nPort);
SOCKET establecerConexionServidor         (in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *dir);

void rutinaAtencionCliente         (void *sock);
int rutinaCrearThread               (void *(*funcion)(void *), threadArgs *args);

int EnviarFormularioHtml            (SOCKET sockfd, msgGet getInfo);
int EnviarRespuestaHtml             (SOCKET socket, msgGet getInfo, void *respuesta,
                                        unsigned long respuestaLen, struct timeb tiempoInicio);
int EnviarRespuestaHtmlCache        (SOCKET socket, char *htmlCode, msgGet getInfo);

int generarHtmlWEB                  (int htmlFile, so_URL_HTML      *respuesta, unsigned long respuestaLen);
int generarHtmlOTROS                (int htmlFile, so_URL_Archivos  *respuesta, unsigned long respuestaLen);
int generarFinDeHtml                (int htmlFile);
int generarEncabezadoHtml           (int htmlFile, msgGet getInfo,
                                        unsigned int cantidad, struct timeb tiempoInicio);

int solicitarBusqueda               (SOCKET sockQP, msgGet getInfo, void **respuesta, unsigned long *respuestaLen, int *mode);
int solicitarBusquedaCache          (SOCKET sockQP, msgGet getInfo, hostsCodigo **respuesta, int *mode);

configuracion config;
mutex_t logMutex;

/*
 * 
 */
int main() 
{

    SOCKET sockFrontEnd;
    mode_t modeOpen = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    /*Se inicializa el mutex*/
    mutex_init(&logMutex, USYNC_THREAD, NULL);
    
    WriteLog(config->log, "Front-end", getpid(), thr_self(), "Inicio de ejecucion", "INFO");

    /*Lectura de Archivo de Configuracion*/
    WriteLog(config->log, "Front-end", getpid(), thr_self(), "Se leera archivo de configuracion", "INFO");
    if (leerArchivoConfiguracion(&config) != 0)
       rutinaDeError("Lectura Archivo de configuracion", config->log);
    WriteLog(config->log, "Front-end", getpid(), thr_self(), "Leido OK", "INFOFIN");

    /*Se establece conexion a puerto de escucha*/
    WriteLog(config->log, "Front-end", getpid(), thr_self(), "Se establecera conexion de escucha", "INFO");
    if ((sockFrontEnd = establecerConexionEscucha(INADDR_ANY, config.puertoL)) == INVALID_SOCKET)
       rutinaDeError("Socket invalido", config->log);
    WriteLog(config->log, "Front-end", getpid(), thr_self(), "Establecida OK", "INFOFIN");

    /*Hasta que llegue el handshake del QM*/
    while (1)
    {
        SOCKET sockCliente;
        SOCKADDR_IN dirCliente;
        int nAddrSize = sizeof(dirCliente);
        char buffer[BUF_SIZE];
        char descID[DESCRIPTORID_LEN];
        int mode = IRC_HANDSHAKE_QM;

        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Esperando Query Manager para estar operativo...", "INFOFIN");

        /*Conecto nuevo cliente, posiblemente el QM*/
        sockCliente = accept(sockFrontEnd, (SOCKADDR *) &dirCliente, &nAddrSize);

        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Conexion establecida. Realizando Handshake", "INFO");

        /*Si el socket es invalido, ignora y vuelve a empezar*/
        if (sockCliente == INVALID_SOCKET || (ircRequest_recv (sockCliente, buffer, descID, &mode) < 0))
        {
            WriteLog(config->log, "Front-end", getpid(), thr_self(), "Error", "ERROR");
            if (sockCliente != INVALID_SOCKET)
                close(sockCliente);
            continue;
        }
        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Realizado OK", "INFOFIN");

        /*Guarda en la estructura de configuracion global el ip y puerto del Query Manager*/
        config.ipQM = inet_addr(strtok(buffer, ":"));
        config.puertoQM = htons(atoi(strtok(NULL, "")));

        close(sockCliente);
        break;
    }

    while(1)
    {
        SOCKET sockCliente;
        SOCKADDR_IN dirCliente;
        int nAddrSize = sizeof(dirCliente);
        msgGet getInfo;
        int getType;

        memset(&getInfo, '\0', sizeof(msgGet));

        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Esperando nuevas peticiones...", "INFOFIN");
        putchar('\n');

        /*Acepta la conexion entrante*/
        sockCliente = accept(sockFrontEnd, (SOCKADDR *) &dirCliente, &nAddrSize);
        
        /*Si el socket es invalido, ignora y vuelve a empezar*/
        if (sockCliente == INVALID_SOCKET)
            continue;
        {
            char text[60];
            sprintf(text, "Conexion aceptada de %s", inet_ntoa(dirCliente.sin_addr));
            WriteLog(config->log, "Front-end", getpid(), thr_self(), text, "INFOFIN");
        }

        /*Se recibe el Http GET del cliente*/
        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Se recibira Http GET del cliente", "INFO");
        if (httpGet_recv(sockCliente, &getInfo) < 0)
        {
            WriteLog(config->log, "Front-end", getpid(), thr_self(), "Error al recibir HTTP GET", "ERROR");
            putchar('\n');
            close(sockCliente);
            continue;
        }
        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Recibido OK", "INFOFIN");

        getType = obtenerGetType(getInfo.palabras);
				if (getType == CACHE)
					getInfo.searchType = SEARCH_CACHE;

        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Se atendera cliente", "INFOFIN");
        if (getType == BROWSER)
        /*Si el GET corresponde a un browser pidiendo formulario*/
        {
            int control;
            char text[150];

            /*Envia el formulario html para empezar a atender.*/
            WriteLog(config->log, "Front-end", getpid(), thr_self(), "Se envia Formulario Html", "INFOFIN");
            control = EnviarFormularioHtml (sockCliente, getInfo);
            sprintf(text, "Atencion de cliente finalizada%s", control < 0? " con Error": "");
            WriteLog(config->log, "Front-end", getpid(), thr_self(), text, control<0? "ERROR": "INFOFIN");
            putchar('\n');
            
            close(sockCliente);
        }
        else if (getType == FORMULARIO || getType == CACHE)
        /*Si el GET corresponde a un formulario pidiendo una busqueda*/
        {
            threadArgs args;

            args.socket = sockCliente;
            args.getInfo = getInfo;
            args.dir = dirCliente;

            /*Crea el thread que atendera al nuevo cliente*/
            if (rutinaCrearThread((void *)rutinaAtencionCliente, &args) < 0)
            {
                char text[60];

                sprintf(text, "No se ha podido atender cliente de %s. Se cierra conexion", inet_ntoa(dirCliente.sin_addr));
                WriteLog(config->log, "Front-end", getpid(), thr_self(), text, "ERROR");
                putchar('\n');
                close(sockCliente);
            }
        }
    }

    /*
     * ¿Esperar por todos los threads?
     */
     
    close(sockFrontEnd);

    /*Finalizo el mutex*/
    mutex_destroy(&logMutex);
	close(config->log);

    return (EXIT_SUCCESS);
}


int solicitarBusquedaCache(SOCKET sock, msgGet getInfo, hostsCodigo **respuesta, int *mode)
{
    char descriptorID[DESCRIPTORID_LEN];
    int modeSend = IRC_REQUEST_CACHE;
    unsigned long respuestaLen;

    memset(descriptorID, '\0', DESCRIPTORID_LEN);

    /*Enviar consulta cache a QM*/
    if (ircRequest_send(sock, (void *) &getInfo, sizeof(getInfo), descriptorID, modeSend) < 0)
      return -1;

    *mode = 0x00;

    /*Recibir respuesta cache de QM*/
    if (ircResponse_recv(sock, (void *)respuesta, descriptorID, &respuestaLen, mode) < 0)
        return -1;
    
    return 0;
}

/*
Descripcion: Solicita al QP la busqueda de ciertas palabras claves
Ultima modificacion: Scheinkman, Mariano
Recibe: socket del QP,, msg Get a buscar, estructura de respuestas vacia
 *      y su longitud
Devuelve: ok? 0: -1. Estructura de respuestas llena.
*/
int solicitarBusqueda(SOCKET sock, msgGet getInfo, void **respuesta, unsigned long *respuestaLen, int *mode)
{
    char descriptorID[DESCRIPTORID_LEN];
    int modeSend = 0x00;

    memset(descriptorID, '\0', DESCRIPTORID_LEN);

    if (getInfo.searchType == WEB)      modeSend = IRC_REQUEST_HTML;
    if (getInfo.searchType == IMG)      modeSend = IRC_REQUEST_ARCHIVOS;
    if (getInfo.searchType == OTROS)    modeSend = IRC_REQUEST_ARCHIVOS;

    /*Enviar consulta a QM*/
    if (ircRequest_send(sock, (void *) &getInfo, sizeof(getInfo), descriptorID, modeSend) < 0)
    {
      printf("Error al enviar consulta a QM.\n\n");
      return -1;
    }

    modeSend = 0x00;

    /*Recibir consulta de QM*/
    if (ircResponse_recv(sock, respuesta, descriptorID, respuestaLen, &modeSend) < 0)
    {
        printf("Error al enviar consulta a QM.\n\n");
        return -1;
    }

    *mode = modeSend;

    return 0;
}


int EnviarRespuestaHtmlCache (SOCKET socket, char *htmlCode, msgGet getInfo)
{
    int nBytes = 0, htmlLen = 0;

    /*Se envia Http Ok*/
    if (httpOk_send(socket, getInfo) < 0)
    {
        printf("Error al enviar HTTP OK.\n\n");
        return -1;
    }
		
		htmlLen = strlen(htmlCode);

		nBytes = EnviarBloque(socket, htmlLen, (void *)htmlCode);
		if (nBytes != htmlLen)
    {
        printf("Error al enviar Archivo Cache.\n\n");
        return -1;
    }

    return 0;
}

/*
Descripcion: Atiende al cliente
Ultima modificacion: Scheinkman, Mariano
Recibe: un argumento que contiene socket, msg Get y direccion del Cliente
Devuelve: ok? 0: -1
*/
void rutinaAtencionCliente (void *args)
{
    threadArgs *arg = (threadArgs *) args;
    SOCKET sockCliente = arg->socket;
    msgGet getThread = arg->getInfo, getInfo;
    SOCKADDR_IN dirCliente = arg->dir;
    SOCKET sockQM;
    SOCKADDR_IN dirQM;
    int mode = 0x00;
    char text[60];
		int control;

    void *respuesta = NULL;
    unsigned long respuestaLen = 0;
    struct timeb tiempoInicio;

    memset(&getInfo, '\0', sizeof(msgGet));

    if (getThread.searchType != SEARCH_CACHE)
        sprintf (text, "Se comienza a atender Request de %s", inet_ntoa(dirCliente.sin_addr));
    else
        sprintf (text, "Se comienza a atender Request Cache de %s", inet_ntoa(dirCliente.sin_addr));
    WriteLog(config->log, "Front-end", getpid(), thr_self(), text, "INFOFIN");

    /*Se obtiene el tiempo de inicio de la busqueda*/
    ftime(&tiempoInicio);

    /*Se obtiene el query string a buscar*/
    if (getThread.searchType != SEARCH_CACHE)
    {
        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Obteniendo QueryString", "INFO");
        control = obtenerQueryString(getThread, &getInfo);
    }
    else
    {
        WriteLog(config->log,"Front-end", getpid(), thr_self(), "Obteniendo UUID", "INFO");
        control = obtenerUUID(getThread, &getInfo);
    }
    if (control < 0)
    {
        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Error de tipo", "ERROR");

        /*Si hubo un error, envia Http Not Found y cierra conexion*/
        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Se enviara Http Not Found", "INFO");
        if (httpNotFound_send(sockCliente, getInfo) < 0)
            WriteLog(config->log, "Front-end", getpid(), thr_self(), "Error", "ERROR");
        else
            WriteLog(config->log, "Front-end", getpid(), thr_self(), "Enviado OK", "INFOFIN");

        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Atencion de cliente finalizada", "INFOFIN");
        putchar('\n');
        close(sockCliente);
        thr_exit(NULL);
    }
    else
    {
        if (getThread.searchType != SEARCH_CACHE)
            sprintf(text, "Obtenido OK.\n\tPalabras buscadas: %s\n\tTipo: %d", getInfo.palabras, getInfo.searchType);
        else
            sprintf(text, "Obtenido OK.\n\tUUID a buscar: %s", getInfo.palabras);
        WriteLog(config->log, "Front-end", getpid(), thr_self(), text, "INFOFIN");
    }

     /*Se establece conexion con el Query Manager*/
    WriteLog(config->log, "Front-end", getpid(), thr_self(), "Se establecera conexion con Query Manager", "INFO");
    if ((sockQM = establecerConexionServidor(config.ipQM, config.puertoQM, &dirQM)) == INVALID_SOCKET)
    {
        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Error", "ERROR");
        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Atencion de cliente finalizada", "INFOFIN");
        putchar('\n');
        close(sockCliente);
        thr_exit(NULL);
    }
    else
        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Establecida OK", "INFOFIN");

    /*Reserva un bloque de memoria para poder recibir las respuestas*/
    if (getInfo.searchType == WEB)
        respuesta = (so_URL_HTML *)      malloc (sizeof(so_URL_HTML));
    if (getInfo.searchType == IMG)
        respuesta = (so_URL_Archivos *)  malloc (sizeof(so_URL_Archivos));
    if (getInfo.searchType == OTROS)
        respuesta = (so_URL_Archivos *)  malloc (sizeof(so_URL_Archivos));
    if (getInfo.searchType == SEARCH_CACHE)
        respuesta = (hostsCodigo *)      malloc (sizeof(hostsCodigo));

    /*Se solicita la busqueda al Query Manager y se recibe las respuestas*/
    WriteLog(config->log, "Front-end", getpid(), thr_self(), "Se enviara peticion al Query Manager", "INFO");
    control = 0;
    if (getThread.searchType != SEARCH_CACHE)
        control = solicitarBusqueda(sockQM, getInfo, &respuesta, &respuestaLen, &mode);
    else
        control = solicitarBusquedaCache(sockQM, getInfo, (hostsCodigo**)(&respuesta), &mode);
    if (control < 0)
    {
        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Error", "ERROR");
        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Atencion de cliente finalizada", "INFOFIN");
        putchar('\n');
        close(sockCliente);
        close(sockQM);
        thr_exit(NULL);
    }
	else
		WriteLog(config->log, "Front-end", getpid(), thr_self(), "Respuesta obtenida", "INFOFIN");

    /*Se cierra conexion con el Query Manager*/
    close(sockQM);

    if (mode == IRC_RESPONSE_HTML || mode == IRC_RESPONSE_ARCHIVOS)
    {
        /*Se envia el Html de respuesta al Cliente*/
        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Se enviara respuesta al cliente", "INFO");
        if (EnviarRespuestaHtml(sockCliente, getInfo, respuesta,
                                respuestaLen, tiempoInicio) < 0)
        {
            WriteLog(config->log, "Front-end", getpid(), thr_self(), "Error", "ERROR");

            WriteLog(config->log, "Front-end", getpid(), thr_self(), "Se enviara Http Not Found", "INFO");
            if (httpNotFound_send(sockCliente, getInfo) < 0)
                WriteLog(config->log, "Front-end", getpid(), thr_self(), "Error", "ERROR");
            else
                WriteLog(config->log, "Front-end", getpid(), thr_self(), "Enviado OK", "INFOFIN");
        }
        else
            WriteLog(config->log, "Front-end", getpid(), thr_self(), "Respuesta enviada OK", "INFOFIN");
    }
    else if (mode == IRC_RESPONSE_CACHE)
    {
        /*Se envia el Html de respuesta al Cliente*/
        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Se enviara respuesta al cliente", "INFO");
        if (EnviarRespuestaHtmlCache(sockCliente, ((hostsCodigo*)respuesta)->html, getInfo) < 0)
        {
            WriteLog(config->log, "Front-end", getpid(), thr_self(), "Error", "ERROR");

            /*Si hubo un error, envia Http Not Found y cierra conexion*/
            WriteLog(config->log, "Front-end", getpid(), thr_self(), "Se enviara Http Not Found", "INFO");
            if (httpNotFound_send(sockCliente, getInfo) < 0)
                WriteLog(config->log, "Front-end", getpid(), thr_self(), "Error", "ERROR");
            else
                WriteLog(config->log, "Front-end", getpid(), thr_self(), "Enviado OK", "INFOFIN");
        }
        else
            WriteLog(config->log, "Front-end", getpid(), thr_self(), "Respuesta enviada OK", "INFOFIN");
    }
    else if (mode == IRC_RESPONSE_ERROR)
    {
        WriteLog(config->log, "Front-end", getpid(), thr_self(), "No se a aceptado peticion", "INFOFIN");

        /*Se envia al cliente Http Internal Service Error*/
        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Se enviara Http Internal Service Error", "INFO");
        if (httpInternalServiceError_send(sockCliente, getInfo) < 0)
            WriteLog(config->log, "Front-end", getpid(), thr_self(), "Error", "ERROR");
        else
            WriteLog(config->log, "Front-end", getpid(), thr_self(), "Enviado OK", "INFOFIN");
    }

    WriteLog(log, "Front-end", getpid(), thr_self(), "Atencion de cliente finalizada", "INFOFIN");
    putchar('\n');
    
    free(respuesta);

    /*Se cierra conexion con Cliente*/
    close(sockCliente);
    thr_exit(NULL);
}



/*
Descripcion: Escribe en el archivo html las respuestas para busqueda
 *              de paginas html
Ultima modificacion: Scheinkman, Mariano
Recibe: El fd del archivo Html, las respuestas y su longitud
Devuelve: ok? 0: -1
*/
int generarHtmlWEB(int htmlFile, so_URL_HTML *respuesta, unsigned long respuestaLen)
{
    int i;
    
    for (i=0;i<respuestaLen/sizeof(so_URL_HTML);i++)
    {
        int nBytes;
        char buffer[BUF_SIZE];

        memset(buffer,'\0',sizeof(buffer));
        sprintf(buffer, "<b>%d</b>.<br/>Titulo: %s<br/>"
                    "Descripcion: %s<br/>"
                    "Link: "
                    "<a href=\"%s\">%s</a><br/>"
                    "En cache: "
                    "<a href=\"http://%s:%d/cache=%s\">http://%s:%d/cache=%s</a><br/>"
                    "<br/>",
                    i+1, respuesta[i].titulo, respuesta[i].descripcion,
                    respuesta[i].URL, respuesta[i].URL,
                    inet_ntoa(*(IN_ADDR *) &config.ipL), ntohs(config.puertoL),
                    respuesta[i].UUID, inet_ntoa(*(IN_ADDR *) &config.ipL),
                    ntohs(config.puertoL), respuesta[i].UUID);

        lseek(htmlFile,0L,2);
        nBytes = write(htmlFile, buffer, strlen(buffer));
        if (nBytes != strlen(buffer))
        {
            perror("write: Html file");
            return -1;
        }
    }

    return 0;
}


/*
Descripcion: Escribe en el archivo html las respuestas para busqueda
 *              de imagenes u otros archivos
Ultima modificacion: Scheinkman, Mariano
Recibe: El fd del archivo Html, las respuestas y su longitud
Devuelve: ok? 0: -1
*/
int generarHtmlOTROS(int htmlFile, so_URL_Archivos *respuesta, unsigned long respuestaLen)
{
    int i;

    for (i=0;i<respuestaLen/sizeof(so_URL_Archivos);i++)
    {
        int nBytes;
        char buffer[MAX_HTML];

        memset(buffer,'\0',sizeof(buffer));
        sprintf(buffer, "<b>%d</b>.<br/>Nombre: %s<br/>"
                    "Formato: %s<br/>"
                    "Size: %s<br/>"
                    "Link: <a href=\"%s\">%s</a><br/>"
                    , i+1, respuesta[i].nombre, respuesta[i].formato
                    , respuesta[i].length, respuesta[i].URL, respuesta[i].URL);

        lseek(htmlFile,0L,2);
        nBytes = write(htmlFile, buffer, strlen(buffer));
        if (nBytes != strlen(buffer))
        {
            perror("write: Html file");
            return -1;
        }
    }
}



/*
Descripcion: Escribe en el html el encabezado Html
Ultima modificacion: Scheinkman, Mariano
Recibe: El fd del archivo, el msg Get del cliente, la cantidad
 *      de respuestas y el tiempo de inicio de la busqueda.
Devuelve: ok? 0: -1
*/
int generarEncabezadoHtml(int htmlFile, msgGet getInfo,
                           unsigned int cantidad, struct timeb tiempoInicio)
{
    int nBytes;
    int secTiempoRespuesta;
    int milliTiempoRespuesta;
    struct timeb tiempoFinal;
    char buffer[MAX_HTML*2];
    char contenidoHtml[MAX_HTML];
    char tipoDeResultado[20];

    memset(buffer,'\0', sizeof(buffer));
    memset(contenidoHtml,'\0', sizeof(contenidoHtml));
    memset(tipoDeResultado,'\0', sizeof(tipoDeResultado));

    /*Calcula el tiempo total de la busqueda*/
    ftime(&tiempoFinal);
    secTiempoRespuesta = tiempoFinal.time - tiempoInicio.time;
    milliTiempoRespuesta = tiempoFinal.millitm - tiempoInicio.millitm;
    if (milliTiempoRespuesta < 0)
    {
        secTiempoRespuesta++;
        milliTiempoRespuesta = abs(milliTiempoRespuesta);
    }

    /*Obtiene el tipo de busqueda como string*/
    itoa_SearchType(getInfo.searchType, tipoDeResultado);

    sprintf(contenidoHtml,"<html><head><title>%s</title></head><body>", "SOogle - Resultados de Busqueda");
    strcpy(buffer,contenidoHtml);

    sprintf(contenidoHtml, "<p>Resultados obtenidos:</p>"
                    "<p>Tipo de resultados: %s</p>"
                    "<p>Cantidad: %d</p>"
                    "<p>Tiempo de respuesta: %d,%2d segundos</p>"
                    "<p>Palabras: \"<b>%s</b>\"</p>"
                    "<p>-------------------------------------------------</p>"
                    , tipoDeResultado, cantidad
                    , secTiempoRespuesta, milliTiempoRespuesta
                    , getInfo.palabras);

    strcat(buffer,contenidoHtml);

    lseek(htmlFile,0L,0);
    if ((nBytes = write(htmlFile, buffer, strlen(buffer))) == -1)
    {
	     perror("write: htmlFile");
	     return -1;
    }
    if (nBytes != strlen(buffer))
    {
        perror("write: no completo escritura");
        return -1;
    }
    return 0;
}


/*
Descripcion: Escribe en el archivo html el encabezado de fin de Html
Ultima modificacion: Scheinkman, Mariano
Recibe: El file descriptor del archivo Html
Devuelve: ok? 0: -1
*/
int generarFinDeHtml(int htmlFile)
{
    char buffer[MAX_HTML];
    int nBytes;

    memset(buffer,'\0',sizeof(buffer));
    strcpy(buffer, "-------------------------------------------------<br/><br/>"
    					 "</body></html>");

    lseek(htmlFile,0L,2);
    nBytes = write(htmlFile, buffer, strlen(buffer));
    if (nBytes != strlen(buffer))
    {
        perror("write: Html file");
        return -1;
    }
}



/*
Descripcion: Envia Html de respuestas al Cliente
Ultima modificacion: Scheinkman, Mariano
Recibe: Socket y msg Get del Cliente, estructura con las respuestas, longitud de
 *      la estructura con las respuesta y tiempo de inicio de busqueda.
Devuelve: ok? 0: -1
*/
int EnviarRespuestaHtml(SOCKET socket, msgGet getInfo, void *respuesta,
                        unsigned long respuestaLen, struct timeb tiempoInicio)
{
    char tmpFile[MAX_PATH];
    int htmlFile;
    int control;
    int cantidadRespuestas;

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    sprintf(tmpFile, "resultados%d.txt", socket);

    /*Crea o trunca (si ya existe) el archivo a enviar al Cliente en modo escritura*/
    if ((htmlFile = open(tmpFile, O_CREAT | O_WRONLY, mode)) < 0)
    {
        if ((htmlFile = open(tmpFile, O_TRUNC | O_WRONLY, mode)) < 0)
        {
            perror("open htmlFile");
            return -1;
        }
    }

    /*Se calcula la cantidad de rtas para imprimir en el encabezado Html*/
    cantidadRespuestas = getInfo.searchType == WEB?
        respuestaLen/sizeof(so_URL_HTML):respuestaLen/sizeof(so_URL_Archivos);

    /*Escribe en el archivo el encabezado Html*/
    if (generarEncabezadoHtml(htmlFile, getInfo, cantidadRespuestas, tiempoInicio) < 0)
    {
        perror("Generar encabezado Html");
        return -1;
    }

    if (respuestaLen == 0)
        /*Si no hubo respuestas lo escribe en el html*/
        control = generarHtmlVACIO(htmlFile);
    else
    {
        /*Escribe en el html las respuestas, segun el tipo de busqueda.*/
        if (getInfo.searchType == WEB)
            control = generarHtmlWEB(htmlFile, (so_URL_HTML *) respuesta, respuestaLen);
        else if (getInfo.searchType == IMG || getInfo.searchType == OTROS)
            control = generarHtmlOTROS(htmlFile, (so_URL_Archivos *) respuesta, respuestaLen);
        else
        {
            perror("Error de tipo de Busqueda");
            return -1;
        }
    }

    if (control == -1)
    {
        perror("generarHtml: error al escribir resultados");
        return -1;
    }

    /*Escribe en el archivo el fin de Html*/
    if (generarFinDeHtml(htmlFile) < 0)
    {
        perror("Generar fin de Html");
        return -1;
    }

    /*Se cierra archivo y vuelve a abrir en modo lectura*/
    close(htmlFile);
    if ((htmlFile = open(tmpFile, O_RDONLY, mode)) < 0)
    {
        perror("open htmlFile");
        return -1;
    }

    /*Se envia Http Ok*/
    if (httpOk_send(socket, getInfo) < 0)
    {
        printf("Error al enviar HTTP OK.\n\n");
        close(htmlFile);
        return -1;
    }

    /*Se envia archivo*/
    if (EnviarArchivo(socket, htmlFile) != getFileSize(htmlFile))
    {
        printf("Error al enviar Archivo.\n\n");
        close(htmlFile);
        return -1;
    }
    close(htmlFile);

    /*Se borra el archivo una vez enviado*/
    unlink(tmpFile);

    return 0;
}



/*
Descripcion: Escribe en el html que no hubo resultados
Ultima modificacion: Scheinkman, Mariano
Recibe: El file descriptor del archivo Html
Devuelve: ok? 0: -1
*/
int generarHtmlVACIO(int htmlFile)
{
    int nBytes;
    char buffer[MAX_HTML];

    memset(buffer,'\0',sizeof(buffer));
    strcpy(buffer, "No se encontro ningun match <br/>");

    lseek(htmlFile,0L,2);
    nBytes = write(htmlFile, buffer, strlen(buffer));
    if (nBytes != strlen(buffer))
    {
        perror("write: Html file");
        return -1;
    }
    return 0;
}


/*
Descripcion: Transforma una clave numerica en su significado en string
Ultima modificacion: Scheinkman, Mariano
Recibe: La clave numerica.
Devuelve: Su significado en string
*/
void itoa_SearchType(int searchType, char *tipo)
{
    if (searchType == WEB)
        strcpy(tipo, "Paginas Web");
    if (searchType == IMG)
        strcpy(tipo, "Imagenes");
    if (searchType == OTROS)
        strcpy(tipo, "Otros Documentos");
    
    return;
}


/*
Descripcion: Crea el thread que atendera al cliente
Ultima modificacion: Scheinkman, Mariano
Recibe: La funcion handler del thread, socket, msg Get y direccion del Cliente
Devuelve: ok? 0: -1
*/
int rutinaCrearThread(void *(*funcion)(void *), threadArgs *args)
{
    thread_t thr;

    /*Se crea el thread de atencion de cliente*/
    if (thr_create((void *) NULL, /*PTHREAD_STACK_MIN*/ thr_min_stack() +1024, funcion, (void *) args, 0, &thr) < 0)
    {
        printf("Error al crear thread: %d", errno);
        return -1;
    }

    return 0;
}


/*
Descripcion: Envia formulario Html al Cliente
Ultima modificacion: Scheinkman, Mariano
Recibe: Socket del cliente y msg Get que recibido
Devuelve: ok? 0: -1
*/
int EnviarFormularioHtml(SOCKET sockCliente, msgGet getInfo)
{
    /* \
     * HACER  \
     * info en : http://xmlsoft.org/ \
     * usar bibliotecas "libxml2"      \
     */

    int fdFile;

    if ((strcmp(getInfo.palabras, "/index.hmtl") == 0) || (strcmp(getInfo.palabras, "imgs/soogle.jpg") == 0) )
    {
        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Se esperaba recibir \"/SOogle.html\"", "INFOFIN");
        WriteLog(config->log, "Front-end", getpid(), thr_self(), "Se enviara Http Not Found", "INFO");
        if (httpNotFound_send(sockCliente, getInfo) < 0)
        {
            WriteLog(config->log, "Front-end", getpid(), thr_self(), "Error", "ERROR");
            return -1;
        }
        else
            WriteLog(config->log, "Front-end", getpid(), thr_self(), "Enviado OK", "INFOFIN");
        return -1;
    }
    else
    {
        char *filename = getInfo.palabras;
        
        if ((fdFile = open(++filename, O_RDONLY, 0)) < 0)
        {
            perror("open");
            if (httpNotFound_send(sockCliente, getInfo) < 0)
            {
                WriteLog(config->log, "Front-end", getpid(), thr_self(), "Error", "ERROR");
                return -1;
            }
            return -1;
        }
        else
        {
           WriteLog(config->log, "Front-end", getpid(), thr_self(), "Se enviara Http 200 Ok", "INFO");
            if (httpOk_send(sockCliente, getInfo) < 0)
            {
               WriteLog(config->log, "Front-end", getpid(), thr_self(), "Error", "ERROR");
                return -1;
            }
            else
                WriteLog(config->log, "Front-end", getpid(), thr_self(), "Enviado OK", "INFOFIN");

            WriteLog(config->log, "Front-end", getpid(), thr_self(), "Se enviara archivo solicitado", "INFO");
            if (EnviarArchivo(sockCliente, fdFile) != getFileSize(fdFile))
            {
                WriteLog(config->log, "Front-end", getpid(), thr_self(), "Error", "ERROR");
                close(fdFile);
                return -1;
            }
            else
                WriteLog(config->log, "Front-end", getpid(), thr_self(), "Enviado OK", "INFOFIN");
            close(fdFile);
        }
    }

    return 0;
}


/*
Descripcion: Atiende error cada vez que exista en alguna funcion y finaliza proceso.
Ultima modificacion: Scheinkman, Mariano
Recibe: Error detectado.
Devuelve: -
*/
void rutinaDeError(char* error, int log)
{
    printf("\r\nError: %s\r\n", error);
    printf("Codigo de Error: %d\r\n\r\n", errno);
    perror(error);

    /*Mutua exclusion*/
    if (log != 0)
    {
    	WriteLog(log, "Front-end", getpid(), thr_self(), error, "ERROR");
    	close(log);
	}

    exit(EXIT_FAILURE);
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
            return -1;

        /*if (ioctlsocket(sockfd, FIONBIO, &NonBlock) == SOCKET_ERROR)
        {
        return -1;
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
                return -1;
            else
            return sockfd;
        }
        else
            return -1;
    }

    return INVALID_SOCKET;
}


/*
Descripcion: Establece la conexion del front-end con el Query Procesor.
Ultima modificacion: Scheinkman, Mariano
Recibe: Direccion ip y puerto del QP.
Devuelve: ok? Socket del servidor: socket invalido.
*/
SOCKET establecerConexionServidor(in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *their_addr)
{
    SOCKET sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return -1;

    their_addr->sin_family = AF_INET;
    their_addr->sin_port = nPort;
    their_addr->sin_addr.s_addr = nDireccionIP;
    memset(&(their_addr->sin_zero),'\0',8);

    /*if (connect(sockfd, (struct sockaddr *)their_addr, sizeof(struct sockaddr)) == -1)
        return -1;*/

    while ( connect (sockfd, (SOCKADDR *)their_addr, sizeof(SOCKADDR)) == -1 && errno != EISCONN )
        if ( errno != EINTR )
        {
            perror("connect");
            close(sockfd);
            return -1;
    	}

    return sockfd;
}



















