#include "threadlist.h"

extern ptrListaThread listaThread;
extern HANDLE listMutex;

void imprimeLista(ptrListaThread ptr)
{
	if (ptr == NULL)
		printf("(Vacia)\r\n\r\n");
	else
	{
		printf("La lista es:\r\n");
         
		while (ptr != NULL)
		{
			printf("Request %d -> ", ptr->info.socket);
			ptr = ptr->sgte;
		}
		printf("NULL\r\n\r\n");
	}
}

int EliminarThread(SOCKET cli)
{
	ptrListaThread ptrA = listaThread;
	ptrListaThread ptrAnt = NULL;

	/*Se busca el cliente en lista*/
	while (ptrA->info.socket != cli && ptrA != NULL)
	{
		ptrAnt = ptrA;
		ptrA = ptrA->sgte;
	}

	if (ptrA == listaThread)
	{
		WaitForSingleObject(listMutex, INFINITE);
		listaThread = listaThread->sgte;
		ReleaseMutex(listMutex);
	}
	else
		ptrAnt->sgte = ptrA->sgte;
	
	if (ptrA == NULL)
		return -1;

	/*closesocket(ptrA->info.socket);*/
	if (ptrA->info.threadHandle != 0)
		CloseHandle(ptrA->info.threadHandle);
	ptrA->sgte = NULL;
	HeapFree (GetProcessHeap(), 0, ptrA);

	ptrA = NULL;

	return 0;
}

int AgregarThread(ptrListaThread *ths, SOCKET socket, SOCKADDR_IN dirCliente, msgGet getInfo)
{      
	ptrListaThread nuevo, actual, anterior;
       
	if (socket == INVALID_SOCKET) 
	{
		printf("Error en _beginthreadex(): %d\r\n", GetLastError());
		return -1;
	}
       
	if ( (nuevo = HeapAlloc(GetProcessHeap(), 0, sizeof (struct nodoListaThread))) == NULL)
		return -1;

	nuevo->info.threadHandle = 0;
	nuevo->info.socket = socket;
	nuevo->info.direccion = dirCliente;
	nuevo->info.threadID = 0;
	nuevo->info.estado = ESPERA;
	nuevo->info.bytesEnviados = 0;
	nuevo->info.getInfo = getInfo;
	nuevo->info.arrival = GetTickCount();
	nuevo->sgte = NULL;

	anterior = NULL;
	actual = *ths;

	while (actual != NULL)
	{
		anterior = actual;
		actual = actual->sgte;
	}
	
	if (anterior == NULL)
	{
		WaitForSingleObject(listMutex, INFINITE);
		nuevo->sgte = *ths;
		*ths = nuevo;
		ReleaseMutex(listMutex);
	}
	else
	{		anterior->sgte = nuevo;
		nuevo->sgte = actual;
	}

	return 0;
}

int listaVacia(ptrListaThread listaThread)
{
	int boolean;

	if (listaThread == NULL)
		boolean = 1;
	else
		boolean = 0;
        
	return boolean;
}


unsigned cantidadThreadsLista(ptrListaThread listaThread, int mode)
{
	ptrListaThread ptrAux = listaThread;
	int contador = 0;

	while (ptrAux != NULL)
	{
		if (ptrAux->info.estado == mode)
			contador++;
		ptrAux = ptrAux->sgte;
	}

	return contador;
}

ptrListaThread BuscarThreadxId(ptrListaThread listaThread, DWORD threadId)
{
	ptrListaThread ptr = listaThread;

	while (ptr != NULL && ptr->info.threadID != threadId)
		ptr = ptr->sgte;
	return ptr;
}

ptrListaThread BuscarThread(ptrListaThread listaThread, SOCKET sockfd)
{
	ptrListaThread ptr = listaThread;

	while (ptr != NULL)
	{
		if (ptr->info.socket == sockfd && ptr->info.estado != ATENDIDO)
			break;
		ptr = ptr->sgte;
	}
	return ptr;
}

ptrListaThread BuscarProximoThread(ptrListaThread listaThread)
{
	ptrListaThread ptr = listaThread;

	while (ptr != NULL && ptr->info.estado != ESPERA)
		ptr = ptr->sgte;
	return ptr;
}
