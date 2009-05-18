/* 
 * File:   http.h
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 20:09
 */

#ifndef _HTTP_H
#define	_HTTP_H

#include "ld.h"

enum filetype_t {HTML, TXT, PHP, JPG, GIF, PNG, JPEG, PDF, ARCHIVO, EXE, ZIP, DOC, XLS, PPT };

typedef struct {
    char palabras[MAX_PATH];
    int protocolo;
    int searchType;
    char queryString[QUERYSTRING_SIZE];
} msgGet;

typedef struct {
    SOCKET socket;
    msgGet getInfo;
    SOCKADDR_IN dir;
} threadArgs;

int EnviarBloque                        (SOCKET sockfd, unsigned long bAEnviar, void *bloque);
int RecibirBloque                       (SOCKET sockfd, char *bloque);
int RecibirNBloque                      (SOCKET socket, void *buffer, unsigned long length);
int EnviarArchivo                       (SOCKET sockRemoto, int filefd);

int httpGet_recv                        (SOCKET sockfd, msgGet *getInfo, int *getType);
int httpNotFound_send                   (SOCKET sockfd, msgGet getInfo);
int httpOk_send                         (SOCKET sockfd, msgGet getInfo);

int obtenerGetType                      (const char *palabras);
int obtenerQueryString                  (msgGet getThread, msgGet *getInfo);
unsigned long getFileSize               (int fdFile);

#endif	/* _HTTP_H */

