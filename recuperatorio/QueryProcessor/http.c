/* 
 * File:   http.c
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 20:08
 */

#include "http.h"

int detectarCaracter                    (char *palabra);
void desplazarCadena                    (char *ptr, char caracter);
void transformarCaracteresEspeciales    (char *palabra);
void formatQueryString                  (char *palabras, char *queryString);



/*
Descripcion: Envia un bloque de datos
Ultima modificacion: Scheinkman, Mariano
Recibe: socket, longitud del bloque, bloque
Devuelve: ok? bytes enviados: -1
*/
int EnviarBloque(SOCKET sockfd, unsigned long len, void *buffer)
{
    int bHastaAhora = 0;
    int bytesEnviados = 0;

    do {
        if ((bHastaAhora = send(sockfd, buffer+bytesEnviados, len-bHastaAhora, 0)) == -1){
                break;
        }
        bytesEnviados += bHastaAhora;
    } while (bytesEnviados != len);

    return bHastaAhora == -1? -1: bytesEnviados;
}


/*
Descripcion: Recibe n bytes de un bloque de datos
Ultima modificacion: Scheinkman, Mariano
Recibe: socket, bloque, longitud del bloque
Devuelve: ok? bytes recibidos: -1
*/
int RecibirNBloque(SOCKET socket, void *buffer, unsigned long length)
{
    int bRecibidos = 0;
    int bHastaAhora = 0;

    do {
        if ((bHastaAhora = recv(socket, buffer+bRecibidos, length-bHastaAhora, 0)) == -1) {
                break;
        }
        if (bHastaAhora == 0)
                return 0;
        bRecibidos += bHastaAhora;
    } while (bRecibidos != length);

    return bHastaAhora == -1? -1: bRecibidos;
}



/*
Descripcion: Recibe un bloque de datos que no se sabe la longitud
Ultima modificacion: Scheinkman, Mariano
Recibe: socket, bloque
Devuelve: ok? bytes recibidos: -1
*/
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



/*
Descripcion: Recibe un mensaje GET del protocolo Html
Ultima modificacion: Scheinkman, Mariano
Recibe: socket, estructura msgGet vacia, tipo del pedido vacio
Devuelve: ok? 0: -1. Esctructura msgGet y tipo del pedido llenos.
*/
int httpGet_recv(SOCKET sockfd, msgGet *getInfo, int *getType)
{
    char buffer[BUF_SIZE], *ptr;
    int bytesRecv=-1, error = 0;

    memset(buffer, '\0', MAX_HTTP);

    getInfo->searchType = -1;
    memset(getInfo->queryString, '\0', QUERYSTRING_SIZE);

    if ((bytesRecv = RecibirBloque(sockfd, buffer)) == -1)
        error = 1;
    else
    {
        int error = 1;
        char *palabras;

        ptr = buffer;
        for (;*ptr != NULL;ptr++)
        {
            if (!memcmp(ptr,"GET ", strlen("GET ")))
            {
                error = 0;
                ptr = ptr + strlen("GET ");
                palabras = ptr;
            }
            if (!error && *ptr == ' ')
            {
                strncpy(getInfo->palabras, palabras, ptr - palabras);
                getInfo->palabras[ptr - palabras] = '\0';
            }

            if (!error && !memcmp(ptr,"HTTP/1.", strlen("HTTP/1.")))
            {
                ptr = ptr + strlen("HTTP/1.");
                getInfo->protocolo = *ptr - '0';
                break;
            }
        }

        if (getInfo->protocolo != 0 && getInfo->protocolo != 1)
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



/*
Descripcion: Envia un archivo atraves de un socket
Ultima modificacion: Scheinkman, Mariano
Recibe: socket, descriptor del archivo
Devuelve: ok? bytes enviados: -1
*/
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



/*
Descripcion: Recibe un mensaje Not Found del protocolo Html
Ultima modificacion: Scheinkman, Mariano
Recibe: socket, estructura msgGet
Devuelve: ok? 0: -1
*/
int httpNotFound_send(SOCKET sockfd, msgGet getInfo)
{
    char buffer[MAX_HTTP];

    memset(buffer,'\0',MAX_HTTP);

    sprintf(buffer, "HTTP/1.%d 404 Not Found\n\n<b>ERROR</b>: File %s was not found\n",
                    getInfo.protocolo, getInfo.palabras);


    /*Enviamos el buffer como stream (sin el \0)*/
    if (EnviarBloque(sockfd, strlen(buffer), buffer) == -1 )
        return -1;
    else
    {
        printf("Se ha enviado HTTP 404 Not Found\n");
        return 0;
    }
}



/*
Descripcion: Recibe un mensaje Ok del protocolo Html
Ultima modificacion: Scheinkman, Mariano
Recibe: socket, estructura msgGet
Devuelve: ok? 0: -1
*/
int httpOk_send(SOCKET sockfd, msgGet getInfo)
{
    char buffer[MAX_HTTP];
    char tipoArchivo[50];
    int error = 0;
    int fileType;
    unsigned long fileSize;

    memset(buffer,'\0',MAX_HTTP);

    if (getInfo.searchType != -1)
        fileType = HTML;
    else
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



/*
Descripcion: Obtiene el tamaño en bytes de un archivo
Ultima modificacion: Scheinkman, Mariano
Recibe: descriptor del archivo
Devuelve: ok? tamaño: -1
*/
unsigned long getFileSize(int fdFile)
{
    unsigned long size;
    struct stat fileInfo;

    if (fstat(fdFile, &fileInfo) < 0)
    {
        perror("Error de fstat");
        return -1;
    }
    size = (unsigned long) fileInfo.st_size;

    return size;
}



/*
Descripcion: Obtiene el tipo de un archivo
Ultima modificacion: Scheinkman, Mariano
Recibe: nombre del archivo
Devuelve: codigo del tipo
*/
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



/*
Descripcion: Obtiene el tipo de pedido GET
Ultima modificacion: Scheinkman, Mariano
Recibe: palabras recibidas en el GET
Devuelve: codigo del tipo de pedido
*/
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
        if (strstr(palabras, "buscar="))
        {
            return FORMULARIO;
        }
        else
            return BROWSER;
    }
}


/*
Descripcion: Obtiene el query string de una busqueda
Ultima modificacion: Scheinkman, Mariano
Recibe: estructura GET con la busqueda, y estructura GET vacia
Devuelve: ok? 0: -1. Estructura GET llena con el querystring
*/
int obtenerQueryString(msgGet getThread, msgGet *getInfo)
{
    char *palabra, *tipo, *ptr;
    char busqueda[MAX_PATH];
    char queryString[QUERYSTRING_SIZE];

    memset(busqueda, '\0', MAX_PATH);
    memset(queryString, '\0', QUERYSTRING_SIZE);

    strcpy(busqueda, getThread.palabras);
    
    ptr = strstr(busqueda, "buscar=");
    tipo = strstr(busqueda, "&tipo=");

    *tipo++ = '\0';
    tipo= tipo + strlen("tipo=");

    palabra = ptr + strlen("buscar=");

    if (!strcmp(tipo, "web"))
        getInfo->searchType = WEB;
    else if (!strcmp(tipo, "img"))
        getInfo->searchType = IMG;
    else if (!strcmp(tipo, "otros"))
        getInfo->searchType = OTROS;
    else
    {
        strcpy(getInfo->palabras, getThread.palabras);
        return -1;
    }

    transformarCaracteresEspeciales(palabra);
    strcpy(getInfo->palabras, palabra);
    formatQueryString(getInfo->palabras, queryString);

    strcpy(getInfo->queryString, queryString);

    return 0;
}


/*
Descripcion: Formatea el query string segun los filtros utilizados por ldap
Ultima modificacion: Scheinkman, Mariano
Recibe: query string a formatear, query string vacio.
Devuelve: query String formateado
*/
void formatQueryString(char *palabras, char *queryString)
{
    char *ptrInit = palabras;
    char *ptrSeek = palabras;
    char palabra[MAX_PATH];
    char buf[MAX_PATH];

    char andBuf[MAX_PATH];
    int and = 0;
    int or = 0;

    memset(andBuf,'\0',MAX_PATH);

    for (; *ptrSeek != NULL; ptrSeek++)
    {
        if (ptrSeek == palabras)
        {
           if (*ptrSeek != '-' && *ptrSeek != '+')
           {
                memset(palabra,'\0',MAX_PATH);
            	memset(buf,'\0',MAX_PATH);

                ptrInit = ptrSeek;
                while(*ptrSeek != '\0' && *ptrSeek != '+' && *ptrSeek != '-')
                    ptrSeek++;

                strncpy(palabra, ptrInit, ptrSeek-ptrInit);
                palabra[ptrSeek-ptrInit] = '\0';
                strcpy(buf, "(|(utnurlKeywords=");
                strcat(buf,palabra);
                strcat(buf,")");
                strcat(andBuf, buf);
                and = 1;
           }
           if (*ptrSeek == '-')		*ptrSeek = '+';
        }

        if (*ptrSeek == '+')
        {
            memset(palabra,'\0',MAX_PATH);
            memset(buf,'\0',MAX_PATH);

            ptrInit = ++ptrSeek;
            while(*ptrSeek != '\0' && *ptrSeek != '+' && *ptrSeek != '-')
                ptrSeek++;

           strncpy(palabra, ptrInit, ptrSeek-ptrInit);
           sprintf(buf,"%s(utnurlKeywords=%s)", !and? "(|":"", palabra);
           strcat(andBuf, buf);

           if (!and)	and = 1;

           ptrSeek--;
        }
    }
     strcat(andBuf,")");

    ptrInit = palabras;
    ptrSeek = palabras;

    for (; *ptrSeek != NULL; ptrSeek++)
    {
        if (*ptrSeek == '-')
        {
            char palabra[MAX_PATH];
            char buf[MAX_PATH];

            memset(palabra,'\0',MAX_PATH);
            memset(buf,'\0',MAX_PATH);

            ptrInit = ++ptrSeek;
            while(*ptrSeek != '\0' && *ptrSeek != '+' && *ptrSeek != '-')
                ptrSeek++;

            strncpy(palabra, ptrInit, ptrSeek-ptrInit);
            sprintf(buf,"%s%s(!(utnurlKeywords=%s))", !or? "(&":"", !or? andBuf:"", palabra);
            if (!or)   or = 1;
            strcat(queryString, buf);

            ptrSeek--;
        }
    }
    strcat(queryString, or? ")": andBuf);
}



/*
Descripcion: Cambia los caracteres que se modifican por el
 *              protocolo http en hexa, a su correspondiente en ascii
Ultima modificacion: Scheinkman, Mariano
Recibe: palabra
Devuelve: palabra formateada
*/
void transformarCaracteresEspeciales(char *palabra)
{

    for (;*palabra != NULL;palabra++)
    {
        if (*palabra == ' ')    *palabra = '+';
        if (*palabra == '+')    *palabra = '+';
        if (*palabra == '-')    *palabra = '-';

        if (*palabra == '%')
        {
            if (*(palabra+1) == '2')
            {
                if (*(palabra+2) == '0')    desplazarCadena(palabra, '+');
                if (*(palabra+2) == '1')    desplazarCadena(palabra, '!');
                if (*(palabra+2) == '2')    desplazarCadena(palabra, '\"');
                if (*(palabra+2) == '3')    desplazarCadena(palabra, '#');
                if (*(palabra+2) == '4')    desplazarCadena(palabra, '$');
                if (*(palabra+2) == '5')    desplazarCadena(palabra, '%');
                if (*(palabra+2) == '6')    desplazarCadena(palabra, '&');
                if (*(palabra+2) == '7')    desplazarCadena(palabra, '\'');
                if (*(palabra+2) == '8')    desplazarCadena(palabra, '(');
                if (*(palabra+2) == '9')    desplazarCadena(palabra, ')');
                if (*(palabra+2) == 'A')    desplazarCadena(palabra, '*');
                if (*(palabra+2) == 'B')    desplazarCadena(palabra, '+');
                if (*(palabra+2) == 'C')    desplazarCadena(palabra, ',');
                if (*(palabra+2) == 'D')    desplazarCadena(palabra, '-');
                if (*(palabra+2) == 'E')    desplazarCadena(palabra, '.');
                if (*(palabra+2) == 'F')    desplazarCadena(palabra, '/');
            }
            else if (*(palabra+1) == '3')
            {
                if (*(palabra+2) == 'A')    desplazarCadena(palabra, ':');
                if (*(palabra+2) == 'B')    desplazarCadena(palabra, ';');
                if (*(palabra+2) == 'C')    desplazarCadena(palabra, '<');
                if (*(palabra+2) == 'D')    desplazarCadena(palabra, '=');
                if (*(palabra+2) == 'E')    desplazarCadena(palabra, '>');
                if (*(palabra+2) == 'F')    desplazarCadena(palabra, '?');
            }
            else if (*(palabra+1) == '5' && *(palabra+2) == '0')
                 desplazarCadena(palabra, '@');
            else if (*(palabra+1) == '5')
            {
                if (*(palabra+2) == 'B')    desplazarCadena(palabra, '[');
                if (*(palabra+2) == 'C')    desplazarCadena(palabra, '\\');
                if (*(palabra+2) == 'D')    desplazarCadena(palabra, ']');
                if (*(palabra+2) == 'E')    desplazarCadena(palabra, '^');
                if (*(palabra+2) == 'F')    desplazarCadena(palabra, '_');
            }
            else if (*(palabra+1) == '6' && *(palabra+2) == '0')
                 desplazarCadena(palabra, '`');
            else if (*(palabra+1) == '7')
            {
                if (*(palabra+2) == 'B')    desplazarCadena(palabra, '{');
                if (*(palabra+2) == 'C')    desplazarCadena(palabra, '|');
                if (*(palabra+2) == 'D')    desplazarCadena(palabra, '}');
                if (*(palabra+2) == 'E')    desplazarCadena(palabra, '~');
            }
            else    desplazarCadena(palabra,'\\');
        }
    }


}


/*
Descripcion: Desplaza una cadena a la izquierda
Ultima modificacion: Scheinkman, Mariano
Recibe: puntero a la posicion, caracter de control
Devuelve: puntero a la nueva posicion
*/
void desplazarCadena(char *ptr, char caracter)
{
    int i=1;

    if (caracter != '\\')
    {
        ptr[0] = caracter;

        while (ptr[i+2] != NULL)
        {
            ptr[i] = ptr[i+2];
            i++;
        }
        ptr[i] = '\0';
    }
    else
    {
        i=0;
        while (ptr[i+3] != NULL)
        {
            ptr[i] = ptr[i+3];
            i++;
        }
        ptr[i] = '\0';
    }
}







