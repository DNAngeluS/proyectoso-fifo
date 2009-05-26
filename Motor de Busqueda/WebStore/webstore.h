/* 
 * File:   webstore.h
 * Author: marianoyfer
 *
 * Created on 15 de mayo de 2009, 11:47
 */

#ifndef _WEBSTORE_H
#define	_WEBSTORE_H

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

#define SOCKET int
#define SOCKADDR struct sockaddr
#define SOCKADDR_IN struct sockaddr_in
#define INVALID_SOCKET -1
#define MAX_PATH 256
#define SOCKET_ERROR -1
#define BUF_SIZE 1024
#define MAX_HTMLCODE 4096

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
    in_addr_t hostIP;
    in_port_t hostPort;
    unsigned long uts;
} webServerHosts;

#endif	/* _WEBSTORE_H */

