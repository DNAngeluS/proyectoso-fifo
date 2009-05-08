/* 
 * File:   http.c
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 20:08
 */

#include "http.h"


int EnviarBloque(SOCKET sockfd, unsigned long bAEnviar, char *bloque)
{

    unsigned long bHastaAhora, bEnviados;
    int error = 0;
    bHastaAhora = bEnviados = 0;
    
    while (!error && bEnviados < bAEnviar)
    {
        if ((bHastaAhora = send(sockfd, bloque+bEnviados, bAEnviar, 0)) == -1)
        {
            printf("Error en la funcion send.\n\n");
            error = 1;
            break;
        }
        else
        {
            bEnviados += bHastaAhora;
            bAEnviar -= bHastaAhora;
        }
    }

    return error == 0? bEnviados: -1;
}

int RecibirNBloque(SOCKET sockfd, char *bloque, unsigned long nBytes)
{
    int bRecibidos = 0;
    int aRecibir = nBytes;
    int bHastaAhora = 0;

    do
    {
        if ((bHastaAhora = recv(sockfd, bloque+bRecibidos, aRecibir, 0)) == -1)
        {
                printf("Error en la funcion recv.\n\n");
                return -1;
        }
        aRecibir -= bHastaAhora;
        bRecibidos += bHastaAhora;
    } while (aRecibir > 0);

    return bRecibidos;
}


int RecibirBloque(SOCKET sockfd, char *bloque) {

    int bRecibidos = 0;
    int bHastaAhora = 0;
    int NonBlock = 1;

    errno = 0;

    do
    {
        /*Si no es la primera vez lo desbloquea*/
        if (bRecibidos != 0)
        {
            /*Se pone el socket como no bloqueante*/
            if (ioctl(sockfd, FIONBIO, &NonBlock) == SOCKET_ERROR)
            {
                printf("ioctl");
                return -1;
            }
        }
        if ((bHastaAhora = recv(sockfd, bloque+bRecibidos, BUF_SIZE, 0)) == -1 && errno != EWOULDBLOCK)
        {
            printf("Error en la funcion recv.\n\n");
            return -1;
        }
        if (errno == EWOULDBLOCK)
            break;
        bRecibidos += bHastaAhora;
    } while (1);
    
    /*Se vuelve a poner como bloqueante*/
    NonBlock = 0;
    if (ioctl(sockfd, FIONBIO, &NonBlock) == SOCKET_ERROR)
    {
        printf("ioctl");
        return -1;
    }

    return bRecibidos;
}

int httpGet_recv(SOCKET sockfd, msgGet *getInfo, int *getType)
{
    char buffer[BUF_SIZE], *ptr;
    char header[4];
    int bytesRecv=-1, error = 0;

    memset(buffer, '\0', sizeof(buffer));
    memset(header, '\0', sizeof(header));


    if ((bytesRecv = RecibirBloque(sockfd, buffer)) == -1)
        error = 1;
    else
    {
        buffer[bytesRecv+1] = '\0';

        strcpy(header, strtok(buffer, " "));

        if(!strcmp(header, "GET"))
        {
            char *palabras = getInfo->palabras;

            strcpy(palabras, strtok(NULL, " "));
            palabras[strlen(palabras)] = '\0';
            strcpy(getInfo->palabras,palabras);

            *getType = obtenerGetType(palabras);
            
            ptr = strtok(NULL, "HTTP/");
            getInfo->protocolo = atoi((ptr+2));
            if (getInfo->protocolo != 0 && getInfo->protocolo != 1)
                error = 1;          
        }
        else
            error = 1;
    }
    if (error)
    {
        getInfo->protocolo = -1;
        strcpy(getInfo->palabras, "");
        return -1;
    }
    else return 0;
}

int EnviarArchivo(SOCKET sockRemoto, int filefd)
{
    char buf[BUF_SIZE];
    struct stat stat_buf;
    int cantBloques, bUltBloque, i;
    int bEnviadosAux, bEnviadosBloque, bAEnviar, bEnviadosTot;
    int lenALeer, error = 0;

    if (fstat(filefd, &stat_buf) < 0)
    {
        perror("fstat");
        exit(1);
    }
    cantBloques = stat_buf.st_size / BUF_SIZE;
    bUltBloque = stat_buf.st_size % BUF_SIZE;

    for (i=bEnviadosTot=0; !error && i<=cantBloques; i++)
    {
        if (i<cantBloques)
            lenALeer = BUF_SIZE;
        else
            lenALeer = bUltBloque;

        bEnviadosAux = bAEnviar = bEnviadosBloque = 0;

        while (!error && lenALeer != 0)
        {
            bEnviadosAux = bAEnviar = 0;
            if ((bAEnviar = read(filefd, buf, lenALeer)) == -1)
            {
                perror("read");
                error = 1;
                break;
            }
            else
            {
                if ((bEnviadosAux = EnviarBloque(sockRemoto, bAEnviar, buf)) == -1)
                {
                    error = 1;
                }
                bEnviadosBloque += bEnviadosAux;
                lenALeer -= bEnviadosAux;
            }
        }
        bEnviadosTot+=bEnviadosBloque;
    }

    printf("Tamaño de archivo: %d, Enviado: %d\n\n", stat_buf.st_size, bEnviadosTot);
    
    if (stat_buf.st_size != bEnviadosTot)
            error = 1;
    return error==0? bEnviadosTot: -1;
}

int httpNotFound_send(SOCKET sockfd, msgGet getInfo)
{
    char buffer[BUF_SIZE];

    sprintf(buffer, "HTTP/1.%d 404 Not Found\n\n<b>ERROR</b>: File %s was not found\n", getInfo.protocolo, getInfo.palabras);


    /*Enviamos el buffer como stream (sin el \0)*/
    if (EnviarBloque(sockfd, strlen(buffer), buffer) == -1 )
        return -1;
    else
    {
        printf("Se ha enviado HTTP 404 Not Found\n");
        return 0;
    }
}

int httpOk_send(SOCKET sockfd, msgGet getInfo)
{
	
    char buffer[BUF_SIZE];
    char tipoArchivo[50];
    int error = 0;
    int fileType;
    unsigned long fileSize;

    memset(buffer,'\0',BUF_SIZE);

    fileType = getFileType(getInfo.palabras);
    switch (fileType)
    {
    case HTML:
        strcpy(tipoArchivo, "text/html");
        break;
    case TXT:
        strcpy(tipoArchivo, "text/txt");
        break;
    case PHP:
        strcpy(tipoArchivo, "text/php");
        break;
    case JPEG:
        strcpy(tipoArchivo, "image/jpeg");
        break;
    case JPG:
        strcpy(tipoArchivo, "image/jpg");
        break;
    case PNG:
        strcpy(tipoArchivo, "image/png");
        break;
    case GIF:
        strcpy(tipoArchivo, "image/gif");
        break;
    case EXE:
        strcpy(tipoArchivo, "application/octet-stream");
        break;
    case ZIP:
        strcpy(tipoArchivo, "application/zip");
        break;
    case DOC:
        strcpy(tipoArchivo, "application/msword");
        break;
    case XLS:
        strcpy(tipoArchivo, "application/vnd.ms-excel");
        break;
    case PDF:
        strcpy(tipoArchivo, "application/pdf");
        break;
    case PPT:
        strcpy(tipoArchivo, "application/vnd.ms-powerpoint");
        break;
    case ARCHIVO:
        strcpy(tipoArchivo, "application/force-download");
        break;
    }

    if (fileType > JPEG) /*No es ni foto ni texto*/
    {
        /*Crea el Buffer con el protocolo*/
        sprintf(buffer, "HTTP/1.%d 200 OK\nContent-type: %s\n"
                            "Content-Disposition: attachment; filename=\"%s\"\n"
                            "Content-Transfer-Encoding: binary\n"
                            "Accept-Ranges: bytes\n"
                            "Cache-Control: private\n"
                            "Pragma: private\n"
                            "Expires: Mon, 26 Jul 1997 05:00:00 GMT\n"
                            "Content-Length: %ld\n\n", getInfo.protocolo, tipoArchivo, 
                            getInfo.palabras, fileSize);

    }
    else
    {
        sprintf(buffer,"HTTP/1.%d 200 OK\nContent-type: %s\n\n", getInfo.protocolo, tipoArchivo);
    }

    /*Enviamos el buffer como stream (sin el \0)*/
    if (EnviarBloque(sockfd, strlen(buffer), buffer) == -1)
        error = 1;

    printf("Se ha enviado HTTP 200 OK\n");

    return error? -1: 0;
}


int enviarFormularioHtml(SOCKET sockCliente, msgGet getInfo)
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

unsigned long getFileSize(int fdFile)
{
    unsigned long size;
    struct stat fileInfo;

    if (fstat(fdFile, &fileInfo) < 0)
    {
        perror("Error de fstat");
        exit(1);
    }
    size = (unsigned long) fileInfo.st_size;

    return size;
}

int getFileType(const char *nombre)
{
    char *ptr;

    ptr = strrchr(nombre, '.');

    ptr++;

    if (!strcmp(ptr, "jpg"))
        return JPG;
    if (!strcmp(ptr, "txt"))
        return TXT;
    if (!strcmp(ptr, "pdf"))
        return PDF;
    if (!strcmp(ptr, "exe"))
        return EXE;
    if (!strcmp(ptr, "zip"))
        return ZIP;
    if (!strcmp(ptr, "html"))
        return HTML;
    if (!strcmp(ptr, "doc"))
        return DOC;
    if (!strcmp(ptr, "xls"))
        return XLS;
    if (!strcmp(ptr, "ppt"))
        return PPT;
    if (!strcmp(ptr, "gif"))
        return GIF;
    if (!strcmp(ptr, "png"))
        return PNG;
    if (!strcmp(ptr, "jpeg"))
        return JPEG;
    if (!strcmp(ptr, "php"))
        return PHP;
    return ARCHIVO;
}

int obtenerGetType(const char *palabras)
{
    char *ptr;

    ptr = strchr(palabras, '?');

    if (ptr == NULL)
    {
        return BROWSER;
    }
    else
    {
        if (!strcmp(strtok(++ptr, "="),"buscar"))
        {
            return FORMULARIO;
        }
        else
            return BROWSER;
    }
}







