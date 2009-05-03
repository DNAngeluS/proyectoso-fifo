/* 
 * File:   irc.h
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 20:09
 */

#ifndef _IRC_H
#define	_IRC_H

#define MAX_UUID 35

typedef struct {
    char descriptorID [16];
    int payloadDesc;
    long payloadLen;
    void *payload;
}headerIRC;
typedef struct {
    char palabra [MAX_PATH];
    char url [MAX_PATH];
    char titulo [MAX_PATH];
    char desc [MAX_PATH];
    char UUID [MAX_UUID];
}soPalabrasHTML;
typedef struct {
    char palabra [MAX_PATH];
    char url [MAX_PATH];
    int tipo;
    char fomarmato [4];
    long tamano;
}soPalabrasArchivos;
typedef struct {
    char UUID [MAX_UUID];
    char html [MAX_PATH];
}hostsCodigo;
typedef struct {
    char host [MAX_PATH];
    long uts;
}hostsExpiracion;

int ircRequest_send ();
int ircRequest_recv ();
int ircResponse_send ();
int ircResponse_recv ();

#endif	/* _IRC_H */

