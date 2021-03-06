#ifndef _LD_H
#define	_LD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#define MAX_PATH 256
#define SOCKET_ERROR -1
#define BUF_SIZE 4096
#define QUERYSTRING_SIZE 1024

enum getType_t {FORMULARIO, BROWSER};
enum searchType_t {WEB, IMG, OTROS};

typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef unsigned int in_addr_t;
typedef unsigned short in_port_t;
#endif	/* _LD_H */
