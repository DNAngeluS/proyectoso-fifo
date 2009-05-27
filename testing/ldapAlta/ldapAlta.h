/* 
 * File:   ldapAlta.h
 * Author: lucio
 *
 * Created on 26 de mayo de 2009, 18:31
 */

#ifndef _LDAPALTA_H
#define	_LDAPALTA_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "config.h"

#define QUERYSTRING_SIZE 256
#define MAX_PATH 256
#define BUF_SIZE 1024
#define MAX_HTMLCODE 4096
#define MAX_HTML 512
#define MAX_HTTP 512
#define MAX_UUID 35
#define DESCRIPTORID_LEN 16

#define IRC_REQUEST_HTML 0x10
#define IRC_REQUEST_ARCHIVOS 0x11
#define IRC_REQUEST_CACHE 0x12
#define IRC_RESPONSE_HTML 0x20
#define IRC_RESPONSE_ARCHIVOS 0x22
#define IRC_RESPONSE_CACHE 0x22

#define IRC_RESPONSE_HOST 0x23

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
    char host [MAX_PATH];
    long uts;
} hostsExpiracion;

typedef struct {
    char UUID [MAX_UUID];
    char html [MAX_PATH];
} hostsCodigo;

typedef struct {
    char URL            [MAX_PATH];
    char titulo         [MAX_PATH];
    char descripcion    [MAX_PATH];
    char *palabras      [MAX_PATH];
    char htmlCode       [MAX_HTMLCODE];
} crawler_URL_HTML;

typedef struct {
    char URL            [MAX_PATH];
    char length         [20];
    char formato        [MAX_PATH];
    char *palabras      [MAX_PATH];
} crawler_URL_ARCHIVOS;

typedef struct {
    char URL            [MAX_PATH];
    char palabras       [MAX_PATH];
    char titulo         [MAX_PATH];
    char descripcion    [MAX_PATH];
    char htmlCode       [MAX_HTMLCODE];
    char tipo           [2];
    char length         [20];
    char formato        [MAX_PATH];
}webstore_URL;


#endif	/* _LDAPALTA_H */
