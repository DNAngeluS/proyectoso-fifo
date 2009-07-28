/* 
 * File:   querymanager.h
 * Author: marianoyfer
 *
 * Created on 19 de junio de 2009, 11:49
 */

#ifndef _QMANAGER_H
#define	_QMANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/timeb.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/filio.h>
#include <netinet/in.h>
#include <errno.h>
#include <ctype.h>

enum tipoRecurso_t {RECURSO_WEB, RECURSO_ARCHIVOS, RECURSO_AMBOS};

#define INVALID_SOCKET -1
#define MAX_PATH 260
#define SOCKET_ERROR -1
#define BUF_SIZE 1024
#define MAX_HTMLCODE 4096
#define MAX_SIZE_PALABRA 50     /*El tamaño maximo de una palabra dentro de crawler_URL*/
#define MAX_FORMATO 5
#define MAX_UUID 35
#define QUERYSTRING_SIZE 256

/*Definiciones para el manejo de los mensajes de consola*/
#define STR_MSG_HELP "USO:\n\t-help: Desplega modo de uso\n\t-qpstatus: Desplega estado de los Query Processors\
											\n\t-ranking-palabras: Desplega el ranking de palabras mas buscadas\
											\n\t-ranking-recursos: Desplega el ranking de los recursos mas solicitados\n\n"
#define STR_MSG_INVALID_INPUT "Error: Comando invalido. Escriba -help para informacion de comandos\n\n"

typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;
typedef unsigned int in_addr_t;
typedef unsigned short in_port_t;

typedef struct {
    char palabras[MAX_PATH];
    int protocolo;
    int searchType;
    char queryString[QUERYSTRING_SIZE];
} msgGet;

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

#endif	/* _QMANAGER_H */

