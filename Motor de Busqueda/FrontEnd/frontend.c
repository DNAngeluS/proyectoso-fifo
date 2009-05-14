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
SOCKET establecerConexionQP         (in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *dir);

void *rutinaAtencionCliente         (void *sock);
int rutinaCrearThread               (void *(*funcion)(void *), SOCKET sockfd,
                                    msgGet getInfo, SOCKADDR_IN dir);

int EnviarFormularioHtml            (SOCKET sockfd, msgGet getInfo);
int EnviarRespuestaHtml             (SOCKET socket, msgGet getInfo, void *respuesta,
                                        unsigned long respuestaLen, struct timeb tiempoInicio);

int generarHtmlWEB                  (int htmlFile, so_URL_HTML      *respuesta, unsigned long respuestaLen);
int generarHtmlOTROS                (int htmlFile, so_URL_Archivos  *respuesta, unsigned long respuestaLen);
int generarEncabezadoHtml           (int htmlFile, msgGet getInfo,
                                        unsigned long respuestaLen, struct timeb tiempoInicio);

int solicitarBusqueda               (msgGet getInfo,void *respuesta, unsigned long *respuestaLen);

SOCKET sockQP;
SOCKADDR_IN dirQP;
configuracion config;
/*
 * 
 */
int main(int argc, char** argv) {

    SOCKET sockFrontEnd;
    
    /*Lectura de Archivo de Configuracion*/
    if (leerArchivoConfiguracion(&config) != 0)
       rutinaDeError("Lectura Archivo de configuracion");

    /*Se establece conexion con el Query Procesor*/
    if ((sockQP = establecerConexionQP(config.ipQP, config.puertoQP, &dirQP)) == INVALID_SOCKET)
        rutinaDeError("Socket invalido");
    
    printf("Se establecio conexion con el QP satisfactoriamente. "
            "Se comienzan a escuchar conexiones.\n\n");

    /*Se establece conexion a puerto de escucha*/
    if ((sockFrontEnd = establecerConexionEscucha(INADDR_ANY, config.puertoL)) == INVALID_SOCKET)
       rutinaDeError("Socket invalido");

    while(1)
    {
        SOCKET sockCliente;
        SOCKADDR_IN dirCliente;
        int nAddrSize = sizeof(dirCliente);
        msgGet getInfo;
        int getType;

        /*Acepta la conexion entrante*/
        sockCliente = accept(sockFrontEnd, (SOCKADDR *) &dirCliente, &nAddrSize);
        
        /*Si el socket es invalido, ignora y vuelve a empezar*/
        if (sockCliente == INVALID_SOCKET)
            continue;

        printf ("Conexion aceptada de %s.\n", inet_ntoa(dirCliente.sin_addr));

        if (httpGet_recv(sockCliente, &getInfo, &getType) < 0)
        {
            printf("Error al recibir HTTP GET.\n\n");
            close(sockCliente);
            continue;
        }

        if (getType == BROWSER)
        {
            /*Envia el formulario html para empezar a atender.*/
            if (EnviarFormularioHtml (sockCliente, getInfo) < 0)
                printf("No se ha podido atender cliente de %s. Se cierra conexion.\n\n", inet_ntoa(dirCliente.sin_addr));
            close(sockCliente);
            continue;
        }
        else if (getType == FORMULARIO)
        {
            /*Crea el thread que atendera al nuevo cliente*/
            if (rutinaCrearThread(rutinaAtencionCliente, sockCliente, getInfo, dirCliente) < 0)
            {
                printf("No se ha podido atender cliente de %s. Se cierra conexion.\n\n", inet_ntoa(dirCliente.sin_addr));
                close(sockCliente);
            }
            continue;
        }
    }

    /*
     * ¿Esperar por todos los threads?
     */
    
    close(sockQP);
    close(sockFrontEnd);
    return (EXIT_SUCCESS);
}

solicitarBusqueda(msgGet getInfo, void *respuesta, unsigned long *respuestaLen)
{
    char descriptorID[DESCRIPTORID_LEN];

    /*Enviar consulta a QP*/
    if (ircRequest_send(sockQP, (void *) &getInfo, sizeof(getInfo), descriptorID) < 0)
    {
      printf("Error al enviar consulta a QP.\n\n");
      return -1;
    }
    /*Recibir consulta de QP*/
    if (ircResponse_recv(sockQP, respuesta, descriptorID, respuestaLen) < 0)
    {
        printf("Error al enviar consulta a QP.\n\n");
        return -1;
    }
    return 0;
}

void *rutinaAtencionCliente (void *args)
{
    threadArgs *arg = (threadArgs *) args;
    SOCKET sockCliente = arg->socket;
    msgGet getThread = arg->getInfo, getInfo;
    SOCKADDR_IN dirCliente = arg->dir;
    void *respuesta;
    unsigned long respuestaLen;
    struct timeb tiempoInicio;

    printf ("Se comienza a atender Request de %s.\n\n", inet_ntoa(dirCliente.sin_addr));

    ftime(&tiempoInicio);

    /*Se obtiene el query string a buscar*/
    if (obtenerQueryString(getThread, &getInfo) < 0)
    {
        perror("obtenerQueryString: error de tipo");
        if (httpNotFound_send(sockCliente, getInfo) < 0)
        {
            printf("Error al enviar HTTP Not Found.\n\n");
            close(sockCliente);
            thr_exit(NULL);
        }
        close(sockCliente);
        thr_exit(NULL);
    }
    else
         printf("palabras buscadas: %s\ntipo: %d\n", getInfo.palabras, getInfo.searchType);
    
    if (getInfo.searchType == WEB)      respuesta = (so_URL_HTML *)      malloc (sizeof(so_URL_HTML));
    if (getInfo.searchType == IMG)      respuesta = (so_URL_Archivos *)  malloc (sizeof(so_URL_Archivos));
    if (getInfo.searchType == OTROS)    respuesta = (so_URL_Archivos *)  malloc (sizeof(so_URL_Archivos));

    /*Se solicita la busqueda al Query Processor*/
    if (solicitarBusqueda(getInfo, (void *) respuesta, &respuestaLen) < 0)
    {
        perror("Solicitar busqueda");
        close(sockCliente);
        thr_exit(NULL);
    }

    if (EnviarRespuestaHtml(sockCliente, getInfo, (void *) respuesta,
                            respuestaLen, tiempoInicio) < 0)
        perror("Enviar respuesta html");

    free(respuesta);
    close(sockCliente);
    thr_exit(NULL);
}

int generarHtmlWEB(int htmlFile, so_URL_HTML *respuesta, unsigned long respuestaLen)
{
    int i;

    for (i=0;i<respuestaLen/sizeof(so_URL_HTML);i++)
    {
        int nBytes;
        char buffer[BUF_SIZE];
        
        sprintf(buffer, "\t%d. Titulo:%s\n"
                    "\tDescripcion: %s\n"
                    "\tLink: http://%s\n"
                    "\tEn cache: http://%scache=%s\n\n"
                    , i+1, respuesta[i].titulo, respuesta[i].descripcion
                    , respuesta[i].URL, respuesta[i].URL, respuesta[i].UUID);
        
        nBytes = write(htmlFile, buffer, sizeof(buffer));
        if (nBytes != sizeof(buffer))
        {
            perror("write: Html file");
            return -1;
        }
    }

    return 0;
}

int generarHtmlOTROS(int htmlFile, so_URL_Archivos *respuesta, unsigned long respuestaLen)
{
    int i;

    for (i=0;i<respuestaLen/sizeof(so_URL_Archivos);i++)
    {
        int nBytes;
        char buffer[BUF_SIZE];

        sprintf(buffer, "\t%d. Nombre:%s\n"
                    "\tFormato: %s\n"
                    "\tTamaño: %s\n"
                    "\tLink: http://%s\n\n"
                    , i+1, respuesta[i].nombre, respuesta[i].tipo
                    , respuesta[i].length, respuesta[i].URL);

        nBytes = write(htmlFile, buffer, sizeof(buffer));
        if (nBytes != sizeof(buffer))
        {
            perror("write: Html file");
            return -1;
        }
    }

    return 0;
}

int generarEncabezadoHtml(int htmlFile, msgGet getInfo,
                            unsigned long respuestaLen, struct timeb tiempoInicio)
{
    char buffer[BUF_SIZE];
    int nBytes;
    char tipoDeResultado[20];
    int cantidad;
    int secTiempoRespuesta;
    int milliTiempoRespuesta;
    struct timeb tiempoFinal;

    ftime(&tiempoFinal);
    secTiempoRespuesta = tiempoFinal.time - tiempoInicio.time;
    milliTiempoRespuesta = tiempoFinal.millitm - tiempoInicio.millitm;
    itoa_SearchType(getInfo.searchType, tipoDeResultado);
    if (getInfo.searchType == WEB)
        cantidad = respuestaLen/sizeof(so_URL_HTML);
    else
        cantidad = respuestaLen/sizeof(so_URL_Archivos);



    sprintf(buffer, "Resultados obtenidos:\n"
                    "Tipo de resultados: %s\n"
                    "Cantidad: %d\n"
                    "Tiempo de respuesta: %d,%d segundos\n"
                    "Palabras: \"%s\"\n"
                    "-------------------------------------------------\n\n"
                    , tipoDeResultado, cantidad
                    , secTiempoRespuesta, milliTiempoRespuesta
                    , getInfo.palabras);

    nBytes = write(htmlFile, buffer, sizeof(buffer));
    if (nBytes != sizeof(buffer))
    {
        perror("write: Html file");
        return -1;
    }
    return 0;
}

int EnviarRespuestaHtml(SOCKET socket, msgGet getInfo, void *respuesta, 
                        unsigned long respuestaLen, struct timeb tiempoInicio)
{
    int htmlFile;
    int control;

    if ((htmlFile = open("resultados.html", O_RDWR, 0)) < 0)
    {
        perror("open htmlFile");
        return -1;
    }

    if (generarEncabezadoHtml(htmlFile, getInfo, respuestaLen, tiempoInicio) < 0)
    {
        perror("Generar encabezado Html");
        return -1;
    }

    if (getInfo.searchType == WEB)
        control = generarHtmlWEB(htmlFile, (so_URL_HTML *) respuesta, respuestaLen);
    else if (getInfo.searchType == IMG || getInfo.searchType == OTROS)
        control = generarHtmlOTROS(htmlFile, (so_URL_Archivos *) respuesta, respuestaLen);
    else
    {
        perror("Error de tipo de Busqueda");
        return -1;
    }

    /*Se genera el html con la respuesta*/
    if (control == -1)
    {
        perror("generarHtml: error al escribir resultados");

        /*Si hay error al generar Html, enviar not found*/
        if (httpNotFound_send(socket, getInfo) < 0)
            printf("Error al enviar HTTP Not Found.\n\n");
        return -1;
    }

    /*Se envia archivo html*/
    if (httpOk_send(socket, getInfo) < 0)
    {
        printf("Error al enviar HTTP OK.\n\n");
        close(htmlFile);
        return -1;
    }
    if (EnviarArchivo(socket, htmlFile) != getFileSize(htmlFile))
    {
        printf("Error al enviar Archivo.\n\n");
        close(htmlFile);
        return -1;
    }

    close(htmlFile);
    return 0;
}

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

int rutinaCrearThread(void *(*funcion)(void *), SOCKET sockfd, msgGet getInfo, SOCKADDR_IN dir)
{
    thread_t thr;
    threadArgs args;

    args.socket = sockfd;
    args.getInfo = getInfo;
    args.dir = dir;

    if (thr_create((void *) NULL, /*PTHREAD_STACK_MIN*/ thr_min_stack() +1024, funcion, (void *) &args, 0, &thr) < 0)
    {
        printf("Error al crear thread: %d", errno);
        return -1;
    }

    return 0;
}

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
Devuelve: int rutinaCrearThread(void (*funcion)(void *), SOCKET sockfd);-
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
SOCKET establecerConexionQP(in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *their_addr)
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



















