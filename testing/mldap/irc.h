/* 
 * File:   irc.h
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 20:09
 */

#ifndef _IRC_H
#define	_IRC_H


#define MAX_PATH 256
#define MAX_UUID 35
#define DESCRIPTORID_LEN 16

#define IRC_REQUEST_HTML 0x10
#define IRC_REQUEST_ARCHIVOS 0x11
#define IRC_REQUEST_CACHE 0x12
#define IRC_RESPONSE_HTML 0x20
#define IRC_RESPONSE_ARCHIVOS 0x22
#define IRC_RESPONSE_CACHE 0x22

#include "ld.h"


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
    char URL            [MAX_PATH];
    char nombre         [MAX_PATH];
    char tipo           [2];
    char formato        [4];
    char length         [20];
} so_URL_Archivos;

typedef struct {
    char UUID [MAX_UUID];
    char html [MAX_PATH];
} hostsCodigo;

typedef struct {
    char host [MAX_PATH];
    long uts;
} hostsExpiracion;

int ircRequest_send(SOCKET sock, void *bloque, unsigned long bloqueLen, char *descriptorID, int mode);
int ircRequest_recv (SOCKET sock, void *bloque, char *descriptorID, int *mode);
int ircResponse_send (SOCKET sock, char *descriptorID, void *bloque, unsigned long bloqueLen, int mode);
int ircResponse_recv (SOCKET sock, void *bloque, char *descriptorID, unsigned long *respuestaLen, int mode);

#endif	/* _IRC_H */

