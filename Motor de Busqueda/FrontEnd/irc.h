/* 
 * File:   irc.h
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 20:09
 */

#ifndef _IRC_H
#define	_IRC_H

#include "frontend.h"

#define MAX_UUID 35
#define DESCRIPTORID_LEN 16
#define MAX_FORMATO 5

/*Peticiones de busqueda del Front-End*/
#define IRC_REQUEST_HTML 0x10
#define IRC_REQUEST_ARCHIVOS 0x11
#define IRC_REQUEST_CACHE 0x12
#define IRC_REQUEST_UNICOQP 0x13
#define IRC_REQUEST_POSIBLE 0x14

/*Respuestas del QP a peticiones de busqueda*/
#define IRC_RESPONSE_HTML 0x20
#define IRC_RESPONSE_ARCHIVOS 0x21
#define IRC_RESPONSE_CACHE 0x22
#define IRC_RESPONSE_POSIBLE 0x23
#define IRC_RESPONSE_NOT_POSIBLE 0x24

/*QP no esta disponible para busquedas*/
#define IRC_RESPONSE_ERROR 0x25

/*QP realiza handshake para conectarse al QM, al comienzo*/
#define IRC_HANDSHAKE_QP 0x90
#define IRC_HANDSHAKE_QP_OK 0x91
#define IRC_HANDSHAKE_QP_FAIL 0x92

/*QM realiza handshake para conectarse al Front-End, al comienzo*/
#define IRC_HANDSHAKE_QM 0x93



typedef struct {
    char descriptorID [DESCRIPTORID_LEN];
    int payloadDesc;
    long payloadLen;
    void *payload;
} headerIRC;

typedef struct {
    char URL            [MAX_PATH];
    char UUID           [MAX_UUID];
    char palabras       [MAX_PATH];
    char titulo         [MAX_PATH];
    char descripcion    [MAX_PATH];
} so_URL_HTML;

typedef struct {
    char URL [MAX_PATH];
    char nombre [MAX_PATH];
    char palabras [MAX_PATH];
    char tipo [2];
    char formato [MAX_FORMATO];
    char length [20];
} so_URL_Archivos;

typedef struct {
    char UUID [MAX_UUID];
    char html [MAX_HTMLCODE];
} hostsCodigo;

typedef struct {
    char host [MAX_PATH];
    long uts;
} hostsExpiracion;

int ircRequest_send(SOCKET sock, void *bloque, unsigned long bloqueLen, char *descriptorID, int mode);
int ircRequest_recv (SOCKET sock, void *bloque, char *descriptorID, int *mode);
int ircResponse_send (SOCKET sock, char *descriptorID, void *bloque, unsigned long bloqueLen, int mode);
int ircResponse_recv (SOCKET sock, void **bloque, char *descriptorID, unsigned long *respuestaLen, int *mode);

#endif	/* _IRC_H */

