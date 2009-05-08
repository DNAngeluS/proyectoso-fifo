/* 
 * File:   http.h
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 20:09
 */

#ifndef _HTTP_H
#define	_HTTP_H

#include "frontend.h"

enum filetype_t {HTML, TXT, PHP, JPG, GIF, PNG, JPEG, PDF, ARCHIVO, EXE, ZIP, DOC, XLS, PPT };

int EnviarBloque		(SOCKET sockfd, unsigned long bAEnviar, char *bloque);
int RecibirBloque		(SOCKET sockfd, char *bloque);
int RecibirNBloque		(SOCKET sockfd, char *bloque, unsigned long nBytes);

int httpGet_recv                (SOCKET sockfd, msgGet *getInfo, int *getType);
int httpNotFound_send           (SOCKET sockfd, msgGet getInfo);
int httpOk_send                 (SOCKET sockfd, msgGet getInfo);

int enviarFormularioHtml        (SOCKET sockfd, msgGet getInfo);
int EnviarArchivo               (SOCKET sockRemoto, int filefd);
int obtenerGetType              (const char *palabras);
unsigned long getFileSize       (int fdFile);

#endif	/* _HTTP_H */

