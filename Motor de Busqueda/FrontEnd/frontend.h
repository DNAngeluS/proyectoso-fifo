/* 
 * File:   frontend.h
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 19:08
 */

#ifndef _FRONTEND_H
#define	_FRONTEND_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread.h>
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

/*Estructuras para la lista de threads*/
struct thread {
	thread_t threadID;
	SOCKET socket;
	SOCKADDR_IN direccion;
};

typedef struct nodoListaThread{
	struct thread info;
	struct nodoListaThread *sgte;
} NodoListaThread;

typedef NodoListaThread *ptrListaThread;



#endif	/* _FRONTEND_H */

