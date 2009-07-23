#ifndef _QP_H
#define	_QP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/filio.h>
#include <netinet/in.h>
#include <errno.h>

#define INVALID_SOCKET -1
#define MAX_PATH 260
#define SOCKET_ERROR -1
#define BUF_SIZE 1024
#define QUERYSTRING_SIZE 256
#define MAX_HTML 512
#define MAX_HTMLCODE 4096
#define MAX_HTTP 512

enum getType_t {FORMULARIO, BROWSER};
enum searchType_t {WEB, IMG, OTROS, CACHE};
enum tipoRecurso_t {RECURSO_WEB, RECURSO_ARCHIVOS, RECURSO_AMBOS};

typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;
typedef unsigned int in_addr_t;
typedef unsigned short in_port_t;

#endif	/* _QP_H */
