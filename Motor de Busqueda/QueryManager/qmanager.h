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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/timeb.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <setjmp.h>
#include <arpa/inet.h>
#include <netinet/in.h>

enum tipoRecurso_t {RECURSO_WEB, RECURSO_ARCHIVOS, RECURSO_AMBOS};

#define SOCKET int
#define SOCKADDR struct sockaddr
#define SOCKADDR_IN struct sockaddr_in
#define INVALID_SOCKET -1
#define MAX_PATH 256
#define SOCKET_ERROR -1
#define BUF_SIZE 1024
#define MAX_HTMLCODE 4096
#define MAX_SIZE_PALABRA 50     /*El tamaño maximo de una palabra dentro de crawler_URL*/


#endif	/* _QMANAGER_H */

