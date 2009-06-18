#include "connection.h"

extern void rutinaDeError(char *error);

extern ptrListaThread listaThread;
extern configuracion config;
extern infoLogFile infoLog;
extern HANDLE logMutex;
extern HANDLE crawMutex;
extern HANDLE listMutex;
extern int codop;

SOCKET establecerConexionEscucha(in_addr_t nDireccionIP, in_port_t nPort)
{
    SOCKET sockfd;

    /*Obtiene un socket*/
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd != INVALID_SOCKET)
    {
        SOCKADDR_IN addrServidorWeb; /*Address del Web server*/
        char yes = 1;
        /*int NonBlock = 1;*/

        /*Impide el error "addres already in use" y setea non blocking el socket*/
        if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1)
            rutinaDeError("Setsockopt");

        /*if (ioctlsocket(sockfd, FIONBIO, &NonBlock) == SOCKET_ERROR)
        {
        rutinaDeError("ioctlsocket");
        }*/

        addrServidorWeb.sin_family = AF_INET;
        addrServidorWeb.sin_addr.s_addr = nDireccionIP;
        addrServidorWeb.sin_port = nPort;
        memset((&addrServidorWeb.sin_zero), '\0', 8);

        /*Liga socket al puerto y direccion*/
        if ((bind (sockfd, (SOCKADDR *) &addrServidorWeb, sizeof(SOCKADDR_IN))) != SOCKET_ERROR)
        {
            /*Pone el puerto a la escucha de conexiones entrantes*/
            if (listen(sockfd, SOMAXCONN) == -1)
                rutinaDeError("Listen");
            else
            return sockfd;
        }
        else
            rutinaDeError("Bind");
    }

    return INVALID_SOCKET;
}


SOCKET establecerConexionServidor(in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *their_addr)
{
    SOCKET sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        rutinaDeError("socket");

    their_addr->sin_family = AF_INET;
    their_addr->sin_port = nPort;
    their_addr->sin_addr.s_addr = nDireccionIP;
    memset(&(their_addr->sin_zero),'\0',8);

    /*if (connect(sockfd, (struct sockaddr *)their_addr, sizeof(struct sockaddr)) == -1)
        rutinaDeError("connect");*/

    while ( connect (sockfd, (struct sockaddr *)their_addr, sizeof(struct sockaddr)) == -1 && GetLastError() != WSAEISCONN )
        if ( GetLastError() != WSAEINTR )
		{
            fprintf(stderr, "Error en connect: error num %d", GetLastError());
			closesocket(sockfd);
			return -1;
    	}
    
    return sockfd;
}

int enviarPaquete(in_addr_t nDireccion, in_port_t nPort, crawler_URL *paquete, int mode, int palabrasLen)
{
    SOCKET sockWebStore;
    SOCKADDR_IN dirServidor;
    char descriptorID[DESCRIPTORID_LEN];

    /*Se levanta conexion con el Web Store*/
    if ((sockWebStore = establecerConexionServidor(nDireccion, nPort, &dirServidor)) < 0)
        return -1;
    printf("Se establecio conexion con WebStore en %s.\r\n", inet_ntoa(dirServidor.sin_addr));

	/*Se envia el paquete al Web Store*/
	if (ircPaquete_send(sockWebStore, paquete, palabrasLen, descriptorID,  mode) < 0)
    {
        closesocket(sockWebStore);
        return -1;
    }

    printf("Paquete enviado a WebStore en %s.\r\n\r\n", inet_ntoa(dirServidor.sin_addr));

	free(paquete->palabras);
    closesocket(sockWebStore);
    
    return 0;
}

SOCKET rutinaConexionCliente(SOCKET sockWebServer, unsigned maxClientes)
{
	SOCKET sockCliente;					/*Socket del cliente remoto*/
	SOCKADDR_IN dirCliente;				/*Direccion de la conexion entrante*/

	int nAddrSize = sizeof(dirCliente);

	/*Acepta la conexion entrante*/
	sockCliente = accept(sockWebServer, (SOCKADDR *) &dirCliente, &nAddrSize);

	/*Si el servidor No esta fuera de servicio puede atender clientes*/
	if (codop != OUTOFSERVICE)
	{
		if (sockCliente != INVALID_SOCKET) 
		{
			int control = 0;
			msgGet getInfo;

			memset(&getInfo, '\0', sizeof(getInfo));

			printf ("Conexion aceptada de %s:%d.\r\n\r\n", inet_ntoa(dirCliente.sin_addr), ntohs(dirCliente.sin_port));
			/*Recibir el Http GET*/
			if (httpGet_recv(sockCliente, &getInfo) < 0)
			{
				printf("Error al recibir HTTP GET. Se cierra conexion.\r\n\r\n");
				closesocket(sockCliente);
				return -2;
			}
			else
			{

				/*Mutua exclusion para agregar el thread a la lista de threads global*/
				control = AgregarThread(&listaThread, sockCliente, dirCliente, getInfo);
				if (control < 0)
				{
					printf("Memoria insuficiente para nuevo Usuario\r\n\r\n");
					closesocket(sockCliente);
					return -2;
				}
				if (cantidadThreadsLista(listaThread, ATENDIENDOSE) < maxClientes)
				{
					ptrListaThread ptr = BuscarThread(listaThread, sockCliente);
					if (ptr != NULL)
					{
						WaitForSingleObject(listMutex, INFINITE);
						ptr->info.estado = ATENDIENDOSE;
						ReleaseMutex(listMutex);
						
						if (rutinaCrearThread(rutinaAtencionCliente, sockCliente) != 0)
						{
							printf("No se pudo crear thread de Atencion de Cliente\r\n\r\n");
							closesocket(ptr->info.socket);
							return -1;
						}
					}
					else
						rutinaDeError("Inconcistencia en Lista");
				}
				return sockCliente;
			}
		}
		else
		{
			printf("Error en Accept: %d", GetLastError());
			return -1;
		}
	}
	else
	{
		closesocket(sockCliente);
		return -2;
	}
}

void rutinaAtencionCliente (LPVOID args)
{
	SOCKET socket;
	msgGet getInfo;
	SOCKADDR_IN direccion;
	int encontro;
	DWORD bytesEnviados;
	char fileBuscado[MAX_PATH];
	char logMsg[BUF_SIZE];
    int logMsgSize, bytesWritten;
    SYSTEMTIME time;
    DWORD processId, threadId;
	ptrListaThread ptr = listaThread;

	threadId = GetCurrentThreadId();
	do
	{
		ptr = BuscarThreadxId(listaThread, threadId);
	} while (ptr == NULL);

	memcpy(&socket, &ptr->info.socket, sizeof(socket));
	memcpy(&getInfo, &ptr->info.getInfo, sizeof(getInfo));
	memcpy(&direccion, &ptr->info.direccion, sizeof(direccion));

	encontro = bytesEnviados = logMsgSize = bytesWritten = 0;
	
	memset(&time, '\0', sizeof(time));
	memset(logMsg, '\0', sizeof(logMsg));
	memset(fileBuscado, '\0', sizeof(fileBuscado));

	pathUnixToWin(config.directorioFiles, getInfo.filename, fileBuscado);

    processId = GetProcessId(GetCurrentProcess());

    GetLocalTime(&time);
	logMsgSize = sprintf_s(logMsg, BUF_SIZE, "%d/%d/%d %02d:%02d:%02d Web Server %d:"
											"\tSolicitante:\r\n\t%s - User Agent: %s - Recurso: %s.\r\n\r\n",
                                    time.wDay, time.wMonth, time.wYear,
									time.wHour, time.wMinute, time.wSecond, processId,
									inet_ntoa(direccion.sin_addr),
									"(?)" , getInfo.filename);														

	/* Para escribir el archivo log exigimos mutual exception */
	WaitForSingleObject(logMutex, INFINITE);
	infoLog.numRequests++;
	WriteFile(config.archivoLog, logMsg, logMsgSize, &bytesWritten, NULL);
	ReleaseMutex(logMutex);
	
	printf("Filename: %s. Protocolo: %d \r\n", getInfo.filename, getInfo.protocolo);
	encontro = BuscarArchivo(fileBuscado);
	
	if (encontro == -1)
		rutinaDeError("Find file");
	else if (encontro == 1)
	{
		printf("Se encontro archivo en %s.\r\n\r\n", fileBuscado);
		
		if (codop == FINISH)
		{
			closesocket(socket);
			_endthreadex(0);
		}

		if (httpOk_send(socket, getInfo) < 0)
			printf("Error %d al enviar HTTP OK. Se cierra conexion.\r\n\r\n", GetLastError());
		else
		{	
			if ((bytesEnviados = EnviarArchivo(socket, fileBuscado)) < 0)
				printf("Error %d al enviar archivo solicitado. Se cierra conexion.\r\n\r\n", GetLastError());
			else
				printf("Se envio archivo %s correctamente a %s:%d.\r\n\r\n", fileBuscado, 
																	inet_ntoa(direccion.sin_addr),
																	ntohs(direccion.sin_port));
		}
	}
	else
	{
		printf("No se encontro archivo.\r\n\r\n");
		if (httpNotFound_send(socket, getInfo) < 0)
			printf("Error %d al enviar HTTP NOT FOUND. Se cierra conexion.\r\n\r\n", GetLastError());
	}

	/*Mutua exclusion para alterar variable*/
	WaitForSingleObject(logMutex, INFINITE);
	infoLog.numBytes += bytesEnviados;
	ReleaseMutex(logMutex);

	threadId = GetCurrentThreadId();
	do
	{
		ptr = BuscarThreadxId(listaThread, threadId);
	} while (ptr == NULL);

	WaitForSingleObject(listMutex, INFINITE);
	ptr->info.estado = ATENDIDO;
	ReleaseMutex(listMutex);
	closesocket(socket);

	_endthreadex(0);
}

int rutinaCrearThread(void (*funcion)(LPVOID), SOCKET sockfd)
{
	HANDLE threadHandle;
	DWORD threadID;
	ptrListaThread ptr = NULL;

	ptr = BuscarThread(listaThread, sockfd);
	if (ptr == NULL)
		rutinaDeError("Inconcistencia en la Lista");

	printf ("Se comienza a atender Request de %s:%d.\r\n\r\n", inet_ntoa(ptr->info.direccion.sin_addr), ntohs(ptr->info.direccion.sin_port));

	/*Se crea el thread encargado de atender cliente*/
	threadHandle = (HANDLE) _beginthreadex (NULL, 1, (void *) funcion, (LPVOID) &sockfd, 0, &threadID);
	
	/*Se actualiza el nodo*/
	WaitForSingleObject(listMutex, INFINITE);
	ptr->info.threadHandle = threadHandle;
	ptr->info.threadID = threadID;
	ReleaseMutex(listMutex);
		
	if (threadHandle == 0)
		return -1;
	return 0;
}