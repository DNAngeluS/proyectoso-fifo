/* 
 * File:   crawler-create.h
 * Author: marianoyfer
 *
 * Created on 25 de mayo de 2009, 20:02
 */

#ifndef _CRAWLER_CREATE_H
#define	_CRAWLER_CREATE_H

#include <signal.h>
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
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>


typedef unsigned int in_addr_t;
typedef unsigned short in_port_t;


#define SOCKET int
#define SOCKADDR struct sockaddr
#define SOCKADDR_IN struct sockaddr_in
#define INVALID_SOCKET -1
#define MAX_PATH 260
#define SOCKET_ERROR -1
#define BUF_SIZE 1024

#endif	/* _CRAWLER_CREATE_H */

