/* 
 * File:   irc.h
 * Author: marianoyfer
 *
 * Created on 19 de junio de 2009, 20:29
 */

#ifndef _IRC_H
#define	_IRC_H

#include "qmanager.h"

#define MAX_UUID 35
#define DESCRIPTORID_LEN 16
#define MAX_FORMATO 5

/*Peticiones de busqueda del Front-End*/
#define IRC_REQUEST_HTML 0x10
#define IRC_REQUEST_ARCHIVOS 0x11
#define IRC_REQUEST_CACHE 0x12
#define IRC_REQUEST_UNICOQP 0x13

/*Respuestas del QP a peticiones de busqueda*/
#define IRC_RESPONSE_HTML 0x20
#define IRC_RESPONSE_ARCHIVOS 0x21
#define IRC_RESPONSE_CACHE 0x22

/*QP no esta disponible para busquedas*/
#define IRC_RESPONSE_ERROR 0x23

/*QP realiza handshake para conectarse al QM, al comienzo*/
#define IRC_HANDSHAKE_QP 0x90
#define IRC_HANDSHAKE_QP_OK 0x91
#define IRC_HANDSHAKE_QP_FAIL 0x92

/*QM realiza handshake para conectarse al Front-End, al comienzo*/
#define IRC_HANDSHAKE_QM 0x93

typedef struct {
    char descriptorID[DESCRIPTORID_LEN];
    int payloadDesc;
    long payloadLen;
    void *payload;
} headerIRC;

int ircRequest_send(SOCKET sock, void *bloque, unsigned long bloqueLen, char *descriptorID, int mode);
int ircRequest_recv (SOCKET sock, void **bloque, char *descriptorID, int *mode);
int ircResponse_send (SOCKET sock, char *descriptorID, void *bloque, unsigned long bloqueLen, int mode);
int ircResponse_recv (SOCKET sock, void **bloque, unsigned long *respuestaLen, int *mode);

int EnviarBloque                        (SOCKET sockfd, unsigned long bAEnviar, void *bloque);
int RecibirBloque                       (SOCKET sockfd, char *bloque);
int RecibirNBloque                      (SOCKET socket, void *buffer, unsigned long length);

#endif	/* _IRC_H */

