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

#define IRC_REQUEST 0x00
#define IRC_RESPONSE 0x01



typedef struct {
    char descriptorID [DESCRIPTORID_LEN];
    int payloadDesc;
    long payloadLen;
    void *payload;
} headerIRC;

typedef struct {
    char URL        [MAX_PATH];
    char UUID       [MAX_UUID];
    char *palabras  [MAX_PATH];
    char titulo     [MAX_PATH];
    char desc       [MAX_PATH];
} so_URL_HTML;

typedef struct {
    char URL        [MAX_PATH];
    char *palabras  [MAX_PATH];
    char tipo       [2];
    char formato    [4];
    char length     [20];
} so_URL_Archivos;

typedef struct {
    char UUID [MAX_UUID];
    char html [MAX_PATH];
} hostsCodigo;

typedef struct {
    char host [MAX_PATH];
    long uts;
} hostsExpiracion;

int ircRequest_send(SOCKET sock, void *bloque, unsigned long bloqueLen, char *descriptorID);
int ircRequest_recv (SOCKET sock, void *bloque, char *descriptorID);
int ircResponse_send (SOCKET sock, char *descriptorID, void *bloque, unsigned long bloqueLen);
int ircResponse_recv (SOCKET sock, void *bloque, char *descriptorID);

#endif	/* _IRC_H */

