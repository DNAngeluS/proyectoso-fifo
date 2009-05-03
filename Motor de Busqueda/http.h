/* 
 * File:   http.h
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 20:09
 */

#ifndef _HTTP_H
#define	_HTTP_H

#include "frontend.h"

#define MAX_PATH 256

typedef struct {
    char palabras[MAX_PATH];
    int protocolo;
} msgGet;

int EnviarBloque		(SOCKET sockfd, unsigned long bAEnviar, char *bloque);
int RecibirBloque		(SOCKET sockfd, char *bloque);
int RecibirNBloque		(SOCKET sockfd, char *bloque, unsigned long nBytes);

int httpGet_recv                (SOCKET sockfd, msgGet *getInfo);
int enviarFormularioHtml        (SOCKET sockfd);

#endif	/* _HTTP_H */

