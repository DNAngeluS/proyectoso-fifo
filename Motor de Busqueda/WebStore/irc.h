/* 
 * File:   irc.h
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 20:09
 */

#ifndef _IRC_H
#define	_IRC_H

#include "crawler-create.h"

#define MAX_UUID 35
#define DESCRIPTORID_LEN 16

#define IRC_ALTA_HTML 0x10
#define IRC_ALTA_ARCHIVOS 0x11
#define IRC_MODIFICACION_HTML 0x20
#define IRC_MODIFICACION_ARCHIVOS 0x22

#define IRC_CRAWLER_CREATE 0x30
#define IRC_CRAWLER_CONNECT 0x31
#define HANDSHAKE_CRAWLER_CONNECT "SOOGLE CRAWLER CONNECT/1.0\n\n"
#define HANDSHAKE_CRAWLER_OK "CRAWLER OK\n\n"
#define HANDSHAKE_CRAWLER_FAIL "CRAWLER FAIL\n\n"





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
    char palabras       [MAX_PATH];
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
int ircResponse_recv (SOCKET sock, void **bloque, unsigned long *respuestaLen, int *mode);

int EnviarBloque                        (SOCKET sockfd, unsigned long bAEnviar, void *bloque);
int RecibirBloque                       (SOCKET sockfd, char *bloque);
int RecibirNBloque                      (SOCKET socket, void *buffer, unsigned long length);

#endif	/* _IRC_H */

