#ifndef THREADLIST
#define THREADLIST

#ifndef WEBSERVER
	#include "webserver.h"
#endif

/*Estructuras para la cola de threads*/
struct thread {
	DWORD threadID;
	SOCKET socket;
	SOCKADDR_IN direccion;
	HANDLE threadHandle;
	DWORD arrival;
	msgGet getInfo;
	DWORD bytesEnviados;
	int estado;
};

typedef struct nodoListaThread{
	struct thread info;
	struct nodoListaThread *sgte;
} NodoListaThread;

typedef NodoListaThread *ptrListaThread;

void imprimeLista					(ptrListaThread ptr);
int listaVacia						(ptrListaThread listaThread);
int AgregarThread					(ptrListaThread *ths, SOCKET socket, SOCKADDR_IN dirCliente, msgGet getInfo);
int EliminarThread					(SOCKET cli);
unsigned cantidadThreadsLista		(ptrListaThread listaThread, int mode);
ptrListaThread BuscarThread			(ptrListaThread listaThread, SOCKET sockfd);
ptrListaThread BuscarThreadxId		(ptrListaThread listaThread, DWORD threadId);
ptrListaThread BuscarProximoThread	(ptrListaThread listaThread);

#endif