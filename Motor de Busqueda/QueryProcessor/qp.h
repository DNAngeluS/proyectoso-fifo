#ifndef _QP_H
#define	_QP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#define INVALID_SOCKET -1
/*#define SOMAXCONN = 20*/
#define MAX_PATH 256
#define SOCKET_ERROR -1

typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef unsigned int in_addr_t;
typedef unsigned short in_port_t;

#endif	/* _QP_H */
