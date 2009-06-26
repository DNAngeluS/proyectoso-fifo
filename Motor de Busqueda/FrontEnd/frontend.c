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


void rutinaDeError(char *string);
void itoa_SearchType(int searchType, char *tipo);

SOCKET establecerConexionEscucha    (in_addr_t nDireccionIP, in_port_t nPort);
SOCKET establecerConexionServidor         (in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *dir);

void *rutinaAtencionCliente         (void *sock);
void *rutinaAtencionCache           (void *args);
int rutinaCrearThread               (void *(*funcion)(void *), SOCKET sockfd,
                                        msgGet getInfo, SOCKADDR_IN dir);

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

/*
 * 
 */
int main(int argc, char** argv) {

    SOCKET sockFrontEnd;
    
    /*Lectura de Archivo de Configuracion*/
    if (leerArchivoConfiguracion(&config) != 0)
       rutinaDeError("Lectura Archivo de configuracion");


    /*Se establece conexion a puerto de escucha*/
    if ((sockFrontEnd = establecerConexionEscucha(INADDR_ANY, config.puertoL)) == INVALID_SOCKET)
       rutinaDeError("Socket invalido");

    /*Hasta que llegue el handshake del QM*/
    while (1)
    {
        SOCKET sockCliente;
        SOCKADDR_IN dirCliente;
        int nAddrSize = sizeof(dirCliente);
        char buffer[BUF_SIZE];
        char descID[DESCRIPTORID_LEN];
        int mode = IRC_HANDSHAKE_QM;

        printf("Esperando conexion del Query Manager para estar operativo.\n");

        /*Conecto nuevo cliente, posiblemente el QM*/
        sockCliente = accept(sockCliente, (SOCKADDR *) &dirCliente, &nAddrSize);

        /*Si el socket es invalido, ignora y vuelve a empezar*/
        if (sockCliente == INVALID_SOCKET || (ircRequest_recv (sockCliente, buffer, descID, &mode) < 0))
        {
            printf("No se recibio el handshake correctamente. Se cierra conexion.\n\n");
            if (sockCliente != INVALID_SOCKET)
                close(sockCliente);
            continue;
        }

        /*Guarda en la estructura de configuracion global el ip y puerto del Query Manager*/
        config.ipQM = inet_addr(strtok(buffer, ":"));
        config.puertoQM = htons(atoi(strtok(NULL, "")));
        break;
    }

    printf("FRONT-END. Conexion establecida.\n");

    while(1)
    {
        SOCKET sockCliente;
        SOCKADDR_IN dirCliente;
        int nAddrSize = sizeof(dirCliente);
        msgGet getInfo;
        int getType;

        memset(&getInfo, '\0', sizeof(msgGet));
        printf("Esperando conexiones entrantes.\n\n");

        /*Acepta la conexion entrante*/
        sockCliente = accept(sockCliente, (SOCKADDR *) &dirCliente, &nAddrSize);
        
        /*Si el socket es invalido, ignora y vuelve a empezar*/
        if (sockCliente == INVALID_SOCKET)
            continue;

        printf ("Conexion aceptada de %s.\n", inet_ntoa(dirCliente.sin_addr));

        /*Se recibe el Http GET del cliente*/
        if (httpGet_recv(sockCliente, &getInfo, &getType) < 0)
        {
            printf("Error al recibir HTTP GET.\n\n");
            close(sockCliente);
            continue;
        }

        getType = obtenerGetType(getInfo.palabras);

        if (getType == BROWSER)
        /*Si el GET corresponde a un browser pidiendo formulario*/
        {
            /*Envia el formulario html para empezar a atender.*/
            if (EnviarFormularioHtml (sockCliente, getInfo) < 0)
                printf("No se ha podido atender cliente de %s. Se cierra conexion.\n\n", inet_ntoa(dirCliente.sin_addr));
            close(sockCliente);
        }
        else if (getType == FORMULARIO)
        /*Si el GET corresponde a un formulario pidiendo una busqueda*/
        {
            /*Crea el thread que atendera al nuevo cliente*/
            if (rutinaCrearThread(rutinaAtencionCliente, sockCliente, getInfo, dirCliente) < 0)
            {
                printf("No se ha podido atender cliente de %s. Se cierra conexion.\n\n", inet_ntoa(dirCliente.sin_addr));
                close(sockCliente);
            }
        }
        else if (getType == CACHE)
        /*Si el GET corresponde a un cliente pidiendo una pagina Cache*/
        {
            /*Crea el thread que atendera al nuevo cliente*/
            if (rutinaCrearThread(rutinaAtencionCache, sockCliente, getInfo, dirCliente) < 0)
            {
                printf("No se ha podido atender cliente de %s. Se cierra conexion.\n\n", inet_ntoa(dirCliente.sin_addr));
                close(sockCliente);
            }
        }
    }

    /*
     * ¿Esperar por todos los threads?
     */
     
    close(sockFrontEnd);
    return (EXIT_SUCCESS);
}


int solicitarBusquedaCache(SOCKET sockQP, msgGet getInfo, hostsCodigo **respuesta, int *mode)
{
    char descriptorID[DESCRIPTORID_LEN];
    int modeSend = IRC_REQUEST_CACHE;
    unsigned long respuestaLen;

    memset(descriptorID, '\0', DESCRIPTORID_LEN);

    /*Enviar consulta cache a QP*/
    if (ircRequest_send(sockQP, (void *) &getInfo, sizeof(getInfo), descriptorID, modeSend) < 0)
    {
      printf("Error al enviar consulta Cache a QP.\n\n");
      return -1;
    }

    *mode = 0x00;

    /*Recibir respuesta cache de QP*/
    if (ircResponse_recv(sockQP, (void *)respuesta, descriptorID, &respuestaLen, mode) < 0)
    {
        printf("Error al enviar consulta a QP.\n\n");
        return -1;
    }
    return 0;
}

/*
Descripcion: Solicita al QP la busqueda de ciertas palabras claves
Ultima modificacion: Scheinkman, Mariano
Recibe: socket del QP,, msg Get a buscar, estructura de respuestas vacia
 *      y su longitud
Devuelve: ok? 0: -1. Estructura de respuestas llena.
*/
int solicitarBusqueda(SOCKET sockQP, msgGet getInfo, void **respuesta, unsigned long *respuestaLen, int *mode)
{
    char descriptorID[DESCRIPTORID_LEN];
    int modeSend = 0x00;

    memset(descriptorID, '\0', DESCRIPTORID_LEN);

    if (getInfo.searchType == WEB)      modeSend = IRC_REQUEST_HTML;
    if (getInfo.searchType == IMG)      modeSend = IRC_REQUEST_ARCHIVOS;
    if (getInfo.searchType == OTROS)    modeSend = IRC_REQUEST_ARCHIVOS;

    /*Enviar consulta a QP*/
    if (ircRequest_send(sockQP, (void *) &getInfo, sizeof(getInfo), descriptorID, modeSend) < 0)
    {
      printf("Error al enviar consulta a QP.\n\n");
      return -1;
    }

    modeSend = 0x00;

    /*Recibir consulta de QP*/
    if (ircResponse_recv(sockQP, respuesta, descriptorID, respuestaLen, &modeSend) < 0)
    {
        printf("Error al enviar consulta a QP.\n\n");
        return -1;
    }

    *mode = modeSend;

    return 0;
}


int EnviarRespuestaHtmlCache (SOCKET socket, char *htmlCode, msgGet getInfo)
{
    char buffer[strlen(htmlCode) + 1];
    int htmlFile;
    int nBytes;
    char tmpFile[MAX_PATH];

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    sprintf(tmpFile, "cache%d.txt", socket);

    /*Crea o trunca (si ya existe) el archivo a enviar al Cliente en modo escritura*/
    if ((htmlFile = open(tmpFile, O_CREAT | O_WRONLY, mode)) < 0)
    {
        if ((htmlFile = open(tmpFile, O_TRUNC | O_WRONLY, mode)) < 0)
        {
            perror("open htmlFile");
            return -1;
        }
    }

    lseek(htmlFile,0L,0);
    if ((nBytes = write(htmlFile, htmlCode, strlen(htmlCode))) == -1)
    {
	     perror("write: htmlFile");
	     return -1;
	 }
    if (nBytes != strlen(htmlCode))
    {
        perror("write: no completo escritura");
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
Descripcion: Atiende al pedido cache
Ultima modificacion: Scheinkman, Mariano
Recibe: un argumento que contiene socket, msg Get y direccion del Cliente
Devuelve: ok? 0: -1
*/
void *rutinaAtencionCache (void *args)
{
    threadArgs *arg = (threadArgs *) args;
    SOCKET sockCliente = arg->socket;
    msgGet getThread = arg->getInfo, getInfo;
    SOCKADDR_IN dirCliente = arg->dir;
    SOCKET sockQM;
    SOCKADDR_IN dirQM;
    hostsCodigo *respuesta = NULL;
    int mode = 0x00;

    memset(&getInfo, '\0', sizeof(msgGet));
    getInfo.protocolo = getThread.protocolo;

    printf ("Se comienza a atender Request Cache de %s.\n\n", inet_ntoa(dirCliente.sin_addr));

    /*Se obtiene el uuid a buscar*/
    if (obtenerUUID(getThread, &getInfo) < 0)
    {
        perror("obtenerQueryString: error de tipo");

        /*Si hubo un error, envia Http Not Found y cierra conexion*/
        if (httpNotFound_send(sockCliente, getInfo) < 0)
            printf("Error al enviar HTTP Not Found.\n\n");

        close(sockCliente);
        thr_exit(NULL);
    }

    printf("UUID a buscar: %s.\n", getInfo.palabras);

    /*Se establece conexion con el Query Procesor*/
    if ((sockQM = establecerConexionServidor(config.ipQM, config.puertoQM, &dirQM)) == INVALID_SOCKET)
    {
        perror("Establecer conexion QM");
        close(sockCliente);
        thr_exit(NULL);
    }
    printf("Se establecio conexion con el QM satisfactoriamente.");

    /*Se solicita la busqueda al Query Processor y se recibe las respuestas*/
    if (solicitarBusquedaCache(sockQM, getInfo, &respuesta, &mode) < 0)
    {
        perror("Solicitar busqueda");
        close(sockCliente);
        close(sockQM);
        thr_exit(NULL);
    }

    /*Se cierra conexion con el Query Manager*/
    close(sockQM);

    if (mode == IRC_RESPONSE_CACHE)
    {
        /*Se envia el Html de respuesta al Cliente*/
        if (EnviarRespuestaHtmlCache(sockCliente, respuesta->html, getInfo) < 0)
        {
            perror("Enviar respuesta html Cache");

            /*Si hubo un error, envia Http Not Found y cierra conexion*/
            if (httpNotFound_send(sockCliente, getInfo) < 0)
                printf("Error al enviar HTTP Not Found.\n\n");
        }
    }
    else if (mode == IRC_RESPONSE_ERROR)
    {
        if (httpInternalServiceError_send(sockCliente, getInfo) < 0)
            printf("Error al enviar HTTP Internal Service Error.\n\n");
    }

    /*Libero las respuestas ya utlizadas*/
    free(respuesta);

    /*Se cierra conexion con Cliente y con QP*/
    close(sockCliente);
    thr_exit(NULL);
}

/*
Descripcion: Atiende al cliente
Ultima modificacion: Scheinkman, Mariano
Recibe: un argumento que contiene socket, msg Get y direccion del Cliente
Devuelve: ok? 0: -1
*/
void *rutinaAtencionCliente (void *args)
{
    threadArgs *arg = (threadArgs *) args;
    SOCKET sockCliente = arg->socket;
    msgGet getThread = arg->getInfo, getInfo;
    SOCKADDR_IN dirCliente = arg->dir;
    SOCKET sockQM;
    SOCKADDR_IN dirQM;
    int mode = 0x00;

    void *respuesta = NULL;
    unsigned long respuestaLen = 0;
    struct timeb tiempoInicio;

    memset(&getInfo, '\0', sizeof(msgGet));

    printf ("Se comienza a atender Request de %s.\n\n", inet_ntoa(dirCliente.sin_addr));

    /*Se obtiene el tiempo de inicio de la busqueda*/
    ftime(&tiempoInicio);

    /*Se obtiene el query string a buscar*/
    if (obtenerQueryString(getThread, &getInfo) < 0)
    {
        perror("obtenerQueryString: error de tipo");
        
        /*Si hubo un error, envia Http Not Found y cierra conexion*/
        if (httpNotFound_send(sockCliente, getInfo) < 0)
            printf("Error al enviar HTTP Not Found.\n\n");

        close(sockCliente);
        thr_exit(NULL);
    }
    else
         printf("palabras buscadas: %s\ntipo: %d\n", getInfo.palabras, getInfo.searchType);

    /*Se establece conexion con el Query Procesor*/
    if ((sockQM = establecerConexionServidor(config.ipQM, config.puertoQM, &dirQM)) == INVALID_SOCKET)
    {
        perror("Establecer conexion QP");
        close(sockCliente);
        thr_exit(NULL);
    }
    printf("Se establecio conexion con el QP satisfactoriamente.");

    /*Reserva un bloque de memoria para poder recibir las respuestas*/
    if (getInfo.searchType == WEB)      respuesta = (so_URL_HTML *)      malloc (sizeof(so_URL_HTML));
    if (getInfo.searchType == IMG)      respuesta = (so_URL_Archivos *)  malloc (sizeof(so_URL_Archivos));
    if (getInfo.searchType == OTROS)    respuesta = (so_URL_Archivos *)  malloc (sizeof(so_URL_Archivos));

    /*Se solicita la busqueda al Query Processor y se recibe las respuestas*/
    if (solicitarBusqueda(sockQM, getInfo, &respuesta, &respuestaLen, &mode) < 0)
    {
        perror("Solicitar busqueda");
        close(sockCliente);
        close(sockQM);
        thr_exit(NULL);
    }

    /*Se cierra conexion con el Query Manager*/
    close(sockQM);

    if (mode == IRC_RESPONSE_HTML || mode == IRC_RESPONSE_ARCHIVOS)
    {
        /*Se envia el Html de respuesta al Cliente*/
        if (EnviarRespuestaHtml(sockCliente, getInfo, respuesta,
                                respuestaLen, tiempoInicio) < 0)
        {
            perror("Enviar respuesta html");

            /*Si hubo un error, envia Http Not Found y cierra conexion*/
            if (httpNotFound_send(sockCliente, getInfo) < 0)
                printf("Error al enviar HTTP Not Found.\n\n");
        }
    }
    else if (mode == IRC_RESPONSE_ERROR)
    {
        if (httpInternalServiceError_send(sockCliente, getInfo) < 0)
            printf("Error al enviar HTTP Internal Service Error.\n\n");
    }

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
        char buffer[MAX_HTML];

        memset(buffer,'\0',MAX_HTML);
        sprintf(buffer, "<b>%d</b>.<br/>Titulo: %s<br/>"
                    "Descripcion: %s<br/>"
                    "Link: "
                    "<a href=\"%s\">%s</a><br/>"
					"En cache: "
                    "<a href=\"%s\">http://%s/cache=%s</a><br/>"
                    "<br/>",
                    i+1, respuesta[i].titulo, respuesta[i].descripcion,
                    respuesta[i].URL, respuesta[i].URL,
                    respuesta[i].UUID, config.ipL, respuesta[i].UUID);

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

        memset(buffer,'\0',MAX_HTML);
        sprintf(buffer, "<b>%d</b>.<br/>Nombre: %s<br/>"
                    "Formato: %s<br/>"
                    "Size: %s<br/>"
                    "Link: %s<br/><br/>"
                    , i+1, respuesta[i].nombre, respuesta[i].tipo
                    , respuesta[i].length, respuesta[i].URL);

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
    char buffer[MAX_HTML];
    char contenidoHtml[MAX_HTML];
    char tipoDeResultado[20];

    memset(buffer,'\0', MAX_HTML);
    memset(contenidoHtml,'\0', MAX_HTML);
    memset(tipoDeResultado,'\0',20);

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

    memset(buffer,'\0',MAX_HTML);
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

    /*Libero las respuestas ya utlizadas*/
    free(respuesta);

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

    memset(buffer,'\0',MAX_HTML);
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
int rutinaCrearThread(void *(*funcion)(void *), SOCKET sockfd, msgGet getInfo, SOCKADDR_IN dir)
{
    thread_t thr;
    threadArgs args;

    args.socket = sockfd;
    args.getInfo = getInfo;
    args.dir = dir;

    /*Se crea el thread de atencion de cliente*/
    if (thr_create((void *) NULL, /*PTHREAD_STACK_MIN*/ thr_min_stack() +1024, funcion, (void *) &args, 0, &thr) < 0)
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
        printf("Se esperaba recibir \"/SOogle.html\". Se envia HTTP Not Found.\n");
        if (httpNotFound_send(sockCliente, getInfo) < 0)
        {
            printf("Error al enviar HTTP Not Found.\n\n");
            return -1;
        }
        return -1;
    }
    else
    {
        char *filename = getInfo.palabras;
        printf("GTTP GET ok. Se envia %s.\n", getInfo.palabras);


        if ((fdFile = open(++filename, O_RDONLY, 0)) < 0)
        {
            perror("open");
            if (httpNotFound_send(sockCliente, getInfo) < 0)
            {
                printf("Error al enviar HTTP Not Found.\n\n");
                return -1;
            }
            return -1;
        }
        else
        {
            if (httpOk_send(sockCliente, getInfo) < 0)
            {
                printf("Error al enviar HTTP OK.\n\n");
                return -1;
            }

            if (EnviarArchivo(sockCliente, fdFile) != getFileSize(fdFile))
            {
                printf("Error al enviar Archivo.\n\n");
                close(fdFile);
                return -1;
            }
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
void rutinaDeError(char* error)
{
    printf("\r\nError: %s\r\n", error);
    printf("Codigo de Error: %d\r\n\r\n", errno);
    perror(error);
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
        rutinaDeError("socket");

    their_addr->sin_family = AF_INET;
    their_addr->sin_port = nPort;
    their_addr->sin_addr.s_addr = nDireccionIP;
    memset(&(their_addr->sin_zero),'\0',8);

    /*if (connect(sockfd, (struct sockaddr *)their_addr, sizeof(struct sockaddr)) == -1)
        rutinaDeError("connect");*/

    while ( connect (sockfd, (SOCKADDR *)their_addr, sizeof(SOCKADDR)) == -1 && errno != EISCONN )
        if ( errno != EINTR )
        {
            perror("connect");
            close(sockfd);
            return -1;
    	}

    return sockfd;
}



















