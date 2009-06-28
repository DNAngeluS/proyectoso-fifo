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
#define MAX_PATH 256
#define SOCKET_ERROR -1
#define BUF_SIZE 1024
#define MAX_HTMLCODE 4096
#define MAX_SIZE_PALABRA 50     /*El tamaño maximo de una palabra dentro de crawler_URL*/

typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;
typedef unsigned int in_addr_t;
typedef unsigned short in_port_t;

#endif	/* _QMANAGER_H */

