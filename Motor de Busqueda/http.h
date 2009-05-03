/* 
 * File:   http.h
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 20:09
 */

#ifndef _HTTP_H
#define	_HTTP_H

#define MAX_PATH 256

typedef struct {
    char palabras[MAX_PATH];
    int protocolo;
} msgGet;

int httpGet_recv ();

int enviarHtml ();

#endif	/* _HTTP_H */

