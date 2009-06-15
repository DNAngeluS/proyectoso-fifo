#include "config.h"
#include "webserver.h"
#include "http.h"
#include "irc.h"
#include "hash.h"
#include "htmlParser.h"

/*ESTO ES PARA DEBUGUEAR MEMORY LEAKS*/
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
/*ESTO ES PARA DEBUGUEAR MEMORY LEAKS*/
/*Instruccion para ver leaks -->  _CrtDumpMemoryLeaks();   */

/************************=== Funciones ===***********************************/

void rutinaDeError					(char *error);
void rutinaAtencionConsola			(LPVOID args);
void rutinaAtencionCliente			(LPVOID args);

SOCKET rutinaConexionCliente		(SOCKET sockWebServer, unsigned maxClientes);
SOCKET establecerConexionEscucha	(in_addr_t direccionIP, in_port_t nPort);
SOCKET establecerConexionServidor	(in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *their_addr);

void imprimeLista					(ptrListaThread ptr);
int listaVacia						(ptrListaThread listaThread);
int AgregarThread					(ptrListaThread *ths, SOCKET socket, SOCKADDR_IN dirCliente, msgGet getInfo);
void EliminarThread					(SOCKET cli);
unsigned cantidadThreadsLista		(ptrListaThread listaThread);

int rutinaCrearThread				(void (*funcion)(LPVOID), SOCKET sockfd);
ptrListaThread BuscarThread			(ptrListaThread listaThread, SOCKET sockfd);
ptrListaThread BuscarProximoThread	(ptrListaThread listaThread);

int comprobarCondicionesMigracion	(unsigned esperaCrawler);
int rutinaConexionCrawler			(SOCKET sockWebServer);
void rutinaAtencionCrawler			(LPVOID args);
int rutinaTrabajoCrawler			(WIN32_FIND_DATA filedata);
int forAllFiles						(char *directorio, int (*funcion) (WIN32_FIND_DATA));

int generarPaqueteArchivos			(const char *filename, crawler_URL *paquete, int *cantPalabras);
int enviarPaquete					(in_addr_t nDireccion, in_port_t nPort, crawler_URL *paquete, int mode);
int EnviarCrawler					(in_addr_t nDireccion, in_port_t nPort);

int logFinal						(infoLogFile infoLog);
void logInicio						();
/*HACER Genera archivo log con datos estadisticos obtenidos*/



/************************=== Variables Globales ===***********************************/

configuracion config;	/*Estructura con datos del archivo de configuracion*/

infoLogFile infoLog;
HANDLE logMutex;	/*Semaforo de mutua exclusion MUTEX para escribir archivo Log*/

/*HANDLE threadListMutex;	/*Semaforo de mutua exclusion MUTEX para modificar la Lista de Threads*/
ptrListaThread listaThread = NULL;	/*Puntero de la lista de threads de usuarios en ejecucion y espera*/

int codop = RUNNING;	/*
							Codigo de Estado de Servidor.
							0 = running
							1 = finish
							2 = out of service
						*/

HANDLE crawMutex;
int crawPresence = -1;
DWORD crawTimeStamp = 0;

struct nlist *hashtab[HASHSIZE];

/*      
Descripcion: Provee de archivos a clientes solicitantes en la red.
Ultima modificacion: Scheinkman, Mariano
Recibe: -
Devuelve: ok? 0: 1
*/

int main() 
{
	
	WSADATA wsaData;					/*Version del socket*/
	SOCKET sockWebServer;				/*Socket del Web Server*/
	SOCKET sockWebServerCrawler;		/*Socket del Web Server de atencion a Crawlers*/

	HANDLE hThreadConsola;				/*Thread handle de la atencion de Consola*/
	DWORD nThreadConsolaID;				/*ID del thread de atencion de Consola*/

	fd_set fdLectura;
	fd_set fdMaestro;

	int fueradeservicio = 0;

	/*Lectura de Archivo de Configuracion*/
	if (leerArchivoConfiguracion(&config) != 0)
		rutinaDeError("Lectura Archivo de configuracion");
	printf("Se a cargado el Archivo de Configuracion.\r\n");
	
	/*Inicializar Hash Table con datos anteriores, si existen*/
	if (hashLoad() < 0)
		printf("Error tratando de cargar Tabla Hash. Continua ejecucion.\r\n\r\n");

	/*Inicializo variable para archivo Log*/
	/*GetLocalTime(&(infoLog.arrivalUserMode));
	GetSystemTime(&(infoLog.arrivalKernelMode));*/
	infoLog.arrival = GetTickCount();
	infoLog.numBytes = 0;
	infoLog.numRequests = 0;

	/*Inicializacion de los Semaforos Mutex*/
	if ((logMutex = CreateMutex(NULL, FALSE, NULL)) == NULL)
		rutinaDeError("Error en CreateMutex()");
/*	if ((threadListMutex = CreateMutex(NULL, FALSE, NULL)) == NULL)
		rutinaDeError("Error en CreateMutex()");*/
	if ((crawMutex = CreateMutex(NULL, FALSE, NULL)) == NULL)
		rutinaDeError("Error en CreateMutex()");
	
	logInicio();

	/*Inicializacion de los sockets*/
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		rutinaDeError("WSAStartup");

	/*Creacion de Thread de Atencion de Consola*/
	hThreadConsola = (HANDLE) _beginthreadex (NULL, 1, (LPVOID) rutinaAtencionConsola, NULL, 0, &nThreadConsolaID);
	if (hThreadConsola == 0)
		rutinaDeError("Creacion de Thread");

	/*Se establece conexion a puerto de escucha de Crawlers*/
	if ((sockWebServerCrawler = establecerConexionEscucha(config.ip, config.puertoCrawler)) == INVALID_SOCKET)
		rutinaDeError("Socket invalido");

	/*Se establece conexion a puerto de escucha*/
	if ((sockWebServer = establecerConexionEscucha(config.ip, config.puerto)) == INVALID_SOCKET)
		rutinaDeError("Socket invalido");

	printf("\r\n%s", STR_MSG_WELCOME);

	/*Se inicializan los FD_SET para select*/
	FD_ZERO(&fdMaestro);
	FD_SET(sockWebServer, &fdMaestro);
	FD_SET(sockWebServerCrawler, &fdMaestro);

/*------------------------Atencion de Clientes-------------------------*/
	/*Mientras no se indique la finalizacion*/
	while (codop != FINISH)
	{	
		int numSockets;
		struct timeval timeout;

		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		FD_ZERO(&fdLectura);
		memcpy(&fdLectura, &fdMaestro, sizeof(fdMaestro));

		if ((numSockets = select(0, &fdLectura, 0, 0, &timeout)) < 0) 
		{
			rutinaDeError("Select failed");
		}
		
		/*== No hubo actividad -> Select() timeout ==*/
		if (numSockets == 0)
		{
			int cantAtendidos = 0;
			ptrListaThread ptrAux = NULL;

			if (!listaVacia(listaThread))
				ptrAux = listaThread;

			while (ptrAux != NULL)
			{
				if (ptrAux->info.estado == ESPERA && 
					(GetTickCount() - ptrAux->info.arrival) > (config.esperaMaxima))
				{
					ptrListaThread ptrSave = ptrAux->sgte;
					SOCKET sock = ptrAux->info.socket;
								
					printf("Resquet de cliente timed out. Se envia Http Request Timeout.\r\n\r\n");
					if (httpTimeout_send(ptrAux->info.socket, ptrAux->info.getInfo) < 0)
						printf("Error al enviar Http timeout a %s:%d.\r\n\r\n", 
												inet_ntoa(ptrAux->info.direccion.sin_addr),
												ntohs(ptrAux->info.direccion.sin_port));

					closesocket(sock);
					EliminarThread(sock);
					FD_CLR(sock, &fdMaestro);
					ptrAux = ptrSave;
				}
				else
					ptrAux = ptrAux->sgte;
			}
			continue;
		}


		/*== Hay usuario conectandose ==*/
		if (FD_ISSET(sockWebServer, &fdLectura) && codop != FINISH)
		{
			SOCKET sockCliente;

			if ((sockCliente = rutinaConexionCliente(sockWebServer, config.cantidadClientes)) == -1 || sockCliente == -2)
			{
				printf("No se ha podido conectar cliente.%s\r\n", sockCliente == -2? 
					" Servidor esta fuera de servicio.\r\nIngrese -run para reanudar ejecucion.\r\n":"\r\n");
				
				if (sockCliente == -1)
					EliminarThread(sockCliente);
			}
			else
			{
				FD_SET(sockCliente, &fdMaestro);
			}	
		}

		/*== Hay un Crawler queriendo instanciarse ==*/
		else if (FD_ISSET(sockWebServerCrawler, &fdLectura) && codop != FINISH)
		{	
			int control = 0;
			
			if ((control = rutinaConexionCrawler(sockWebServerCrawler)) < 0)
			{
				printf("No se a podido conectar Crawler.");
				if (control == (-2))
					printf(" Servidor esta fuera de servicio.\r\nIngrese -run para reanudar ejecucion.\r\n");
				else if (control == (-3))
					printf(" No se prestan las condiciones de migracion de Crawler.\r\n\r\n");
				else
					printf("\r\n\r\n");

				continue;
			}

		}

		/*== Hay usuario desconectandose ==*/
		else
		{
			ptrListaThread ptrAux = listaThread;
			SOCKET cli;

			while (ptrAux != NULL && !FD_ISSET((cli = ptrAux->info.socket), &fdLectura))
				ptrAux = ptrAux->sgte;
			if (ptrAux == NULL)
				rutinaDeError("Inconcistencia en lista o usuario no desconectado mal detectado");
			else
			{
				WaitForSingleObject(ptrAux->info.threadHandle, INFINITE);
				EliminarThread(cli);
				FD_CLR(cli, &fdMaestro);
				
				/*Se trata de poner algun cliente en espera en atencion*/
				if (cantidadThreadsLista(listaThread) <= config.cantidadClientes)
				{
					ptrListaThread ptr = BuscarProximoThread(listaThread);
					if (ptr != NULL)
					{
						ptr->info.estado = ATENDIDO;
						if (rutinaCrearThread(rutinaAtencionCliente, ptr->info.socket) != 0)
						{
							printf("No se pudo crear thread de Atencion de Cliente\r\n\r\n");
							return -1;
						}
					}
				}
			}
		}
	}
/*--------------------Fin de Atencion de Clientes----------------------*/



	WSACleanup();

	/*Se guarda el estado de la Tabla Hash*/
	if (hashSave() < 0)
		printf("Error al guardar Tabla Hash.\r\n");
	
	if (hashCleanAll() < 0)
		printf("Error al liberar memoria de Tabla Hash.\r\n");

	/*Generar archivo Log*/
	printf("Se genera Archivo Log.\r\n");
	if (logFinal(infoLog) < 0)
		printf("Error generando Archivo Log.\r\n\r\n");
	else
		printf("Archivo Log generado correctamente.\r\n\r\n");
	
	CloseHandle(logMutex);
	CloseHandle(config.archivoLog);

	_CrtDumpMemoryLeaks();

	return 0;
}




/*      
Descripcion: Establece la conexion del servidor web a la escucha en un puerto y direccion ip.
Ultima modificacion: Scheinkman, Mariano
Recibe: Direccion ip y puerto a escuchar.
Devuelve: ok? Socket del servidor: socket invalido.
*/
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



/*      
Descripcion: Thread que atiende los ingresos de usuario por consola.
Ultima modificacion: Scheinkman, Mariano
Recibe: -
Devuelve: 0
*/

void rutinaAtencionConsola (LPVOID args)
{
	/*Mientras no se ingrese la finalizacion*/
	while (codop != FINISH)
	{
		char consolaStdin[MAX_INPUT_CONSOLA];
		int centinela=0;	    

/**------	Validaciones a la hora de ingresar comandos	------**/
		
		gets(consolaStdin);
		if (*consolaStdin == '-')
		{
			if (!lstrcmp(&consolaStdin[1], "queuestatus"))
			{
				printf("%s", STR_MSG_QUEUESTATUS);
				imprimeLista (listaThread);
            }

            else if (!lstrcmp(&consolaStdin[1], "run"))
            {
                 if (codop != RUNNING)
                 {
                    printf("%s", STR_MSG_RUN);
                    codop = RUNNING;
                 }
                 else
                     printf("%s", STR_MSG_INVALID_RUN);
            }

            else if (!lstrcmp(&consolaStdin[1], "finish"))
            {
                 printf("%s", STR_MSG_FINISH);
                 codop = FINISH;
            }

            else if (!lstrcmp(&consolaStdin[1], "outofservice"))
            {
                 if (codop != OUTOFSERVICE)
                 {
                    printf("%s", STR_MSG_OUTOFSERVICE);
                    codop = OUTOFSERVICE;
                 }
                 else
                     printf("%s", STR_MSG_INVALID_OUTOFSERVICE);
            }

			else if (!strncmp(&consolaStdin[1], "private", strlen("private")))
			{
				char *file = NULL, *next = NULL;

				file = strtok_s(consolaStdin, " ", &next);
				file = strtok_s(NULL, "\0", &next);
				
				if (!(file != NULL && isalnum(*file)))
					printf("%s", STR_MSG_INVALID_ARG);
				else
				{
					char path[MAX_PATH];
					DWORD attr;

					sprintf_s(path, MAX_PATH, "%s\\%s", config.directorioFiles, file);
					attr = GetFileAttributes(path);
					
					if (attr == 0)
						rutinaDeError("GetFile Attributes");
					else if (attr == INVALID_FILE_ATTRIBUTES)
						printf("%s", STR_MSG_INVALID_FILE);
					else if (attr == FILE_ATTRIBUTE_READONLY)
						printf("%s", STR_MSG_INVALID_PRIVATE);
					else
					{
						if (SetFileAttributes(path, FILE_ATTRIBUTE_READONLY) == 0)
							rutinaDeError("SetFileAttributes");
						printf("%s", STR_MSG_PRIVATE);
					}
				}
			}

			else if (!strncmp(&consolaStdin[1], "public", strlen("public")))
			{
				char *file = NULL, *next = NULL;

				file = strtok_s(consolaStdin, " ", &next);
				file = strtok_s(NULL, "\0", &next);
				
				if (!(file != NULL && isalnum(*file)))
					printf("%s", STR_MSG_INVALID_ARG);
				else
				{
					char path[MAX_PATH];
					DWORD attr;

					sprintf_s(path, MAX_PATH, "%s\\%s", config.directorioFiles, file);
					attr = GetFileAttributes(path);
					
					if (attr == 0)
						rutinaDeError("GetFile Attributes");
					else if (attr == INVALID_FILE_ATTRIBUTES)
						printf("%s", STR_MSG_INVALID_FILE);
					else if (attr == FILE_ATTRIBUTE_NORMAL)
						printf("%s", STR_MSG_INVALID_PUBLIC);
					else
					{
						if (SetFileAttributes(path, FILE_ATTRIBUTE_NORMAL) == 0)
							rutinaDeError("SetFileAttributes");
						printf("%s", STR_MSG_PUBLIC);
					}
				}
			}

			else if (!lstrcmp(&consolaStdin[1], "files"))
			{
				int i;
				printf("%s", STR_MSG_HASH);
				if (!hashVacia())
				{
					for (i=0; i<HASHSIZE; i++)
						if (hashtab[i] != NULL)
							printf("\t%s : %s\r\n", hashtab[i]->md5, hashtab[i]->file);
				}
				else
					printf("(Vacia)\r\n");
				printf("\r\n");
			}
			else if (!lstrcmp(&consolaStdin[1], "reset"))
			{
				if (hashCleanAll() < 0)
					printf("%s", STR_MSG_INVALID_RESET);
				else
					printf("%s", STR_MSG_RESET);
			}

            else if (!lstrcmp(&consolaStdin[1], "help"))
			   printf("%s", STR_MSG_HELP);

            else
                printf("%s", STR_MSG_INVALID_INPUT);
		}
        else
            printf("%s", STR_MSG_INVALID_INPUT);
	 
	}
	_endthreadex(0);
}

/*      
Descripcion: Atiende error cada vez que exista en alguna funcion y finaliza proceso.
Ultima modificacion: Scheinkman, Mariano
Recibe: Error detectado.
Devuelve: -
*/

void rutinaDeError (char* error) 
{
	printf("\r\nError: %s\r\n", error);
	printf("Codigo de Error: %d\r\n\r\n", GetLastError());

	/*Se guarda el estado de la Tabla Hash*/
	if (hashSave() < 0)
		printf("Error al guardar Tabla Hash.\r\n");

	/*Se limpia de memoria la Tabla Hash*/
	if (hashCleanAll() < 0)
		printf("Error al liberar memoria de Tabla Hash.\r\n");

	_CrtDumpMemoryLeaks();

	exit(1);
}


/*      
Descripcion: Agrega Thread a la lista de threads, encolandolos.
Ultima modificacion: Scheinkman, Mariano
Recibe: Puntero a la lista de threads, el handle del nuevo thread y su ID.
Devuelve: ok? 0: -1
*/

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
		nuevo->sgte = *ths;
		*ths = nuevo;
	}
	else
	{
		anterior->sgte = nuevo;
		nuevo->sgte = actual;
	}

	return 0;
}

void EliminarThread(SOCKET cli)
{
	ptrListaThread ptrAux = listaThread;
	ptrListaThread ptrAnt = NULL;

	/*Se busca el cliente en lista*/
	while (ptrAux->info.socket != cli && ptrAux != NULL)
	{
		ptrAnt = ptrAux;
		ptrAux = ptrAux->sgte;
	}

	if (ptrAux == listaThread)
		listaThread = listaThread->sgte;
	else
		ptrAnt->sgte = ptrAux->sgte;
	
	if (ptrAux == NULL)
		rutinaDeError("Inconcistencia de Threads");

	/*closesocket(ptrAux->info.socket);*/
	if (ptrAux->info.threadHandle != 0)
		CloseHandle(ptrAux->info.threadHandle);
	ptrAux->sgte = NULL;
	HeapFree (GetProcessHeap(), 0, ptrAux);
}


/*      
Descripcion: Thread que Atiende al cliente
Ultima modificacion: Scheinkman, Mariano
Recibe: Socket del cliente
Devuelve: -
*/
void rutinaAtencionCliente (LPVOID args)
{
	int encontro;
	DWORD bytesEnviados = 0;
	struct thread *dataThread = (struct thread *) args;
	char *fileBuscado = pathUnixToWin(config.directorioFiles, dataThread->getInfo.filename);

	char logMsg[BUF_SIZE];
    int logMsgSize, bytesWritten;
    SYSTEMTIME time;
    DWORD id;

    id = GetProcessId(GetCurrentProcess());

    GetLocalTime(&time);
	logMsgSize = sprintf_s(logMsg, BUF_SIZE, "%d/%d/%d %02d:%02d:%02d Web Server %d:"
											"\tSolicitante:\r\n\t%s - User Agent: %s - Recurso: %s.\r\n\r\n",
                                    time.wDay, time.wMonth, time.wYear,
									time.wHour, time.wMinute, time.wSecond, id,
									inet_ntoa(dataThread->direccion.sin_addr),
									"(?)" , dataThread->getInfo.filename);														

	/* Para escribir el archivo log exigimos mutual exception */
	WaitForSingleObject(logMutex, INFINITE);
	infoLog.numRequests++;
	WriteFile(config.archivoLog, logMsg, logMsgSize, &bytesWritten, NULL);
	ReleaseMutex(logMutex);
	
	printf("Filename: %s. Protocolo: %d \r\n", dataThread->getInfo.filename, dataThread->getInfo.protocolo);
	encontro = BuscarArchivo(fileBuscado);
	
	if (encontro)
	{
		printf("Se encontro archivo en %s.\r\n\r\n", fileBuscado);
		
		if (codop == FINISH)
		{
			closesocket(dataThread->socket);
			_endthreadex(0);
		}

		if (httpOk_send(dataThread->socket, dataThread->getInfo) < 0)
			printf("Error al enviar HTTP OK. Se cierra conexion.\r\n\r\n");
		else
		{	
			if ((bytesEnviados = EnviarArchivo(dataThread->socket, fileBuscado)) < 0)
				printf("Error al enviar archivo solicitado. Se cierra conexion.\r\n\r\n");
			else
				printf("Se envio archivo %s correctamente a %s:%d.\r\n\r\n", fileBuscado, 
																	inet_ntoa(dataThread->direccion.sin_addr),
																	ntohs(dataThread->direccion.sin_port));
		}
	}
	else
	{
		printf("No se encontro archivo.\r\n\r\n");
		if (httpNotFound_send(dataThread->socket, dataThread->getInfo) < 0)
			printf("Error al enviar HTTP NOT FOUND. Se cierra conexion.\r\n\r\n");
	}

	closesocket(dataThread->socket);

	/*Mutua exclusion para alterar variable*/
	WaitForSingleObject(logMutex, INFINITE);
	infoLog.numBytes += bytesEnviados;
	ReleaseMutex(logMutex);
	_endthreadex(0);
}

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

int rutinaCrearThread(void (*funcion)(LPVOID), SOCKET sockfd)
{
	HANDLE threadHandle;
	DWORD threadID;
	ptrListaThread ptr = NULL;
	struct thread dataThread;

	ptr = BuscarThread(listaThread, sockfd);
	if (ptr == NULL)
		rutinaDeError("Inconcistencia en la Lista");

	dataThread = ptr->info;
	printf ("Se comienza a atender Request de %s:%d.\r\n\r\n", inet_ntoa(dataThread.direccion.sin_addr), ntohs(dataThread.direccion.sin_port));

	/*Se crea el thread encargado de atender cliente*/
	threadHandle = (HANDLE) _beginthreadex (NULL, 1, (void *) funcion, (LPVOID) &dataThread, 0, &threadID);
	
	/*Se actualiza el nodo*/
	ptr->info.threadHandle = threadHandle;
	ptr->info.threadID = threadID;

		
	if (threadHandle == 0)
		return -1;
	return 0;
}

/*      
Descripcion: Recibe el http get del cliente y lo cuelga de la lista.
Ultima modificacion: Scheinkman, Mariano
Recibe: socket del servidor y el num maximo de clientes
Devuelve: sockCliente si OK. -1 si hay error y agrego a lista. -2 si hay error y no agrego a lista
*/
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
			int control;
			msgGet getInfo;

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
				if (cantidadThreadsLista(listaThread) <= maxClientes)
				{
					ptrListaThread ptr = BuscarThread(listaThread, sockCliente);
					if (ptr != NULL)
					{
						ptr->info.estado = ATENDIDO;
						if (rutinaCrearThread(rutinaAtencionCliente, sockCliente) != 0)
						{
							printf("No se pudo crear thread de Atencion de Cliente\r\n\r\n");
							closesocket(sockCliente);
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

int listaVacia(ptrListaThread listaThread)
{
	int boolean;

	if (listaThread == NULL)
		boolean = 1;
	else
		boolean = 0;
        
	return boolean;
}


unsigned cantidadThreadsLista(ptrListaThread listaThread)
{
	ptrListaThread ptrAux = listaThread;
	int contador = 0;

	while (ptrAux != NULL)
	{
		contador++;
		ptrAux = ptrAux->sgte;
	}

	return contador;
}


ptrListaThread BuscarThread(ptrListaThread listaThread, SOCKET sockfd)
{
	ptrListaThread ptr = listaThread;

	while (ptr != NULL && ptr->info.socket != sockfd)
		ptr = ptr->sgte;
	return ptr;
}

ptrListaThread BuscarProximoThread(ptrListaThread listaThread)
{
	ptrListaThread ptr = listaThread;

	while (ptr != NULL && ptr->info.estado != ESPERA)
		ptr = ptr->sgte;
	return ptr;
}

void logInicio()
{
        char logMsg[BUF_SIZE];
        int logMsgSize, bytesWritten;
        SYSTEMTIME time;
        DWORD id;

        id = GetProcessId(GetCurrentProcess());

        GetLocalTime(&time);
		logMsgSize = sprintf_s(logMsg, BUF_SIZE, "%s%d/%d/%d %02d:%02d:%02d Web Server %d: Inicio de ejecucion.\r\n\r\n",
                                        ENCABEZADO_LOG ,time.wDay, time.wMonth, time.wYear,
										time.wHour, time.wMinute, time.wSecond, id);

        /* Para escribir el archivo log exigimos mutual exception */
        WaitForSingleObject(logMutex, INFINITE);
        WriteFile(config.archivoLog, logMsg, logMsgSize, &bytesWritten, NULL);
        ReleaseMutex(logMutex);
        return;
}

int logFinal(infoLogFile infoLog)
{
	DWORD tiempoTotal;
	DWORD bytesEscritos;
	char buffer[BUF_SIZE];
	int error = 0, logMsgSize;
	float kernel,user;
	FILETIME lpCreationTime,lpExitTime,lpKernelTime,lpUserTime;
    SYSTEMTIME stKernelTime,stUserTime;


	GetProcessTimes(GetCurrentProcess(),&lpCreationTime,&lpExitTime,&lpKernelTime,&lpUserTime);
	FileTimeToSystemTime(&lpUserTime,&stUserTime);
	FileTimeToSystemTime(&lpKernelTime,&stKernelTime);

	kernel = stKernelTime.wMilliseconds;
	kernel = (kernel/1000) + stKernelTime.wHour*3600+ stKernelTime.wMinute*60+ stKernelTime.wSecond;
    
	user = stUserTime.wMilliseconds;
	user = (user/1000) + stUserTime.wHour*3600+ stUserTime.wMinute*60+ stUserTime.wSecond;

	tiempoTotal = GetTickCount() - infoLog.arrival;
	memset(buffer, '\0', BUF_SIZE);
	logMsgSize = sprintf_s(buffer, sizeof(buffer), "%sCantidad de Requests Aceptados: %d\r\n"
						"Cantidad de Bytes Transferidos: %ld\r\n"
						"Tiempo Total de Ejecucion del Servidor (en segundos): %d\r\n"
						"Tiempo Ejecucion:\r\n\tModo Kernel: %f\r\n\tModo Usuario: %f\r\n",
									FINAL_LOG, infoLog.numRequests, infoLog.numBytes,
									tiempoTotal/1000, kernel, user);

	WaitForSingleObject(logMutex, INFINITE);
	if (WriteFile(config.archivoLog, buffer, logMsgSize, &bytesEscritos, NULL) == FALSE)
		error = 1;
	ReleaseMutex(logMutex);

	return error? -1: 0;
}

/*Devuelve ok? socket, -1 si error, -2 si outofservice, -3 si no se cumplen condiciones de migracion*/
int rutinaConexionCrawler(SOCKET sockWebServer)
{
	SOCKET sockCrawler;					/*Socket del cliente remoto*/
	SOCKADDR_IN dirCrawler;				/*Direccion de la conexion entrante*/

	int nAddrSize = sizeof(dirCrawler);

	/*Acepta la conexion entrante*/
	sockCrawler = accept(sockWebServer, (SOCKADDR *) &dirCrawler, &nAddrSize);
	
	/*Si el servidor No esta fuera de servicio puede atender Crawlers*/
	if (sockCrawler != INVALID_SOCKET) 
	{
		if (codop != OUTOFSERVICE)
		{
			int mode = 0x00;
			int comprobacion;
			char descID[DESCRIPTORID_LEN];

			/*Recibir el msg IRC del WebStore*/
			if (ircRequest_recv (sockCrawler, NULL, descID, &mode) < 0)
			{
				printf("Error al recibir IRC Crawler Create. Se cierra conexion.\r\n\r\n");
				closesocket(sockCrawler);
				return (-1);
			}			

			if (mode == IRC_CRAWLER_CONNECT)
			{
				printf ("A migrado un Web Crawler. Se comprobaran Condiciones de Creacion...\r\n");
				
				/*Se comprueban las condiciones para que haya una migracion*/
				if ((comprobacion = comprobarCondicionesMigracion(config.esperaCrawler)) == 1)
				/*Si se puede migrar, envia un Crawler Ok y crea thread*/
				{	
					HANDLE threadHandle;
					DWORD threadID;
					
					ircResponse_send(sockCrawler, descID, IRC_CRAWLER_HANDSHAKE_OK, 
									sizeof(IRC_CRAWLER_HANDSHAKE_OK), IRC_CRAWLER_OK);	
					printf("Migracion de Web Crawler satisfactoria.\r\n\r\n");
					closesocket(sockCrawler);

					/*Se crea el thread Crawler*/
					threadHandle = (HANDLE) _beginthreadex (NULL, 1, (void *) rutinaAtencionCrawler, (LPVOID) NULL, 0, &threadID);
					if (threadHandle == 0)
					{
						printf("No se pudo crear el thread Crawler\r\n\r\n");
					}
					else
					{					
						/*Se modifica el Thread Priority Level*/
						SetThreadPriority(threadHandle, THREAD_PRIORITY_HIGHEST);
					}

					return 0;
				}
				else
				/*Si no, envia un Crawler Fail*/
				{
					ircResponse_send(sockCrawler, descID,IRC_CRAWLER_HANDSHAKE_FAIL, 
									sizeof(IRC_CRAWLER_HANDSHAKE_FAIL), IRC_CRAWLER_FAIL);
					printf("Migracion de Web Crawler a sido rechazada.\r\n\r\n");
					closesocket(sockCrawler);
					return (-3);
				}
			}
/*			else if (mode == IRC_CRAWLER_CREATE)
			{
				HANDLE threadHandle;
				DWORD threadID;
				
				printf ("A migrado un Web Crawler. Se comprobaran Condiciones de Creacion...\r\n");
				closesocket(sockCrawler);

				/*Se crea el thread Crawler*/
/*				threadHandle = (HANDLE) _beginthreadex (NULL, 1, (void *) rutinaAtencionCrawler, (LPVOID) NULL, 0, &threadID);
				if (threadHandle == 0)
				{
					printf("No se pudo crear el thread Crawler\r\n\r\n");
				}
				else
				{					
					/*Se modifica el Thread Priority Level*/
/*					SetThreadPriority(threadHandle, THREAD_PRIORITY_HIGHEST);
				}
				return 0;
			}*/

		}
		closesocket(sockCrawler);
		return (-2);
	}
	else
	{
		printf("Error en Accept: %d", GetLastError());
		return (-1);
	}
	return 0;
}

int comprobarCondicionesMigracion(unsigned esperaCrawler)
{
	return  ((GetTickCount() - crawTimeStamp) >= esperaCrawler) 
			&& (crawPresence <= 0);		
}

void rutinaAtencionCrawler (LPVOID args)
{
	int pvez = (crawPresence == -1);
	int error = 0;
	char logMsg[BUF_SIZE];
    int logMsgSize, bytesWritten;
    SYSTEMTIME time;
    DWORD idProcess;
	DWORD idThread;
	DWORD inicio = GetTickCount();

    idProcess = GetProcessId(GetCurrentProcess());
	idThread = GetCurrentThreadId();

    GetLocalTime(&time);
	logMsgSize = sprintf_s(logMsg, BUF_SIZE, "%d/%d/%d %02d:%02d:%02d Web Crawler %d-%d: Ingreso al sistema.\r\n\r\n",
                                        time.wDay, time.wMonth, time.wYear,
										time.wHour, time.wMinute, time.wSecond, idProcess, idThread);

    /* Para escribir el archivo log exigimos mutual exception */
    WaitForSingleObject(logMutex, INFINITE);
    WriteFile(config.archivoLog, logMsg, logMsgSize, &bytesWritten, NULL);
    ReleaseMutex(logMutex);
	
	_CrtDumpMemoryLeaks();

	/*Mutua exclusion para la variable global de presencia de Crawler*/
	WaitForSingleObject(crawMutex, INFINITE);
	crawPresence = 1;
	ReleaseMutex(crawMutex);
			
	/*Procesar los archivos del directorio*/
	if ((error = forAllFiles(config.directorioFiles, rutinaTrabajoCrawler)) < 0)
		printf("Crawler: Error al procesar archivos. Se descarta Crawler.\r\n\r\n");	
	
	printf("Analisis del Web Crawler a finalizado %s.\r\n\r\n", error<0? "con error.":"satisfactoriamente");
	
	logMsgSize = sprintf_s(logMsg, BUF_SIZE, "%d/%d/%d %02d:%02d:%02d Web Crawler %d-%d: finalizado el analisis de documentos en %d milisegundos.\r\n\r\n",
                                        time.wDay, time.wMonth, time.wYear,
										time.wHour, time.wMinute, time.wSecond, 
										idProcess, idThread, GetTickCount() - inicio);

	WaitForSingleObject(crawMutex, INFINITE);
	WriteFile(config.archivoLog, logMsg, logMsgSize, &bytesWritten, NULL);
	crawTimeStamp = GetTickCount();
	crawPresence = 0;
	ReleaseMutex(crawMutex);

	_endthreadex(0);
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

    while ( connect (sockfd, (struct sockaddr *)their_addr, sizeof(struct sockaddr)) == -1 && errno != WSAEISCONN )
        if ( errno != WSAEINTR )
		{
            fprintf(stderr, "Error en connect: error num %d", GetLastError());
			closesocket(sockfd);
			return -1;
    	}
    
    return sockfd;
}

int forAllFiles(char *directorio, int (*funcion) (WIN32_FIND_DATA))
{
	WIN32_FIND_DATA ffd;
	char szDir[MAX_PATH];
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	/*Chequea la longuitud del directorio*/
	length_of_arg = lstrlen(directorio);

	if (length_of_arg > (MAX_PATH - 2))
	{
		printf("Path del Directorio muy largo.\r\n");
		return -1;
	}

	/*Agrega \* al nombre del directorio*/
	lstrcpy(szDir, directorio);
	lstrcat(szDir, "\\*");

	/*Obtiene el primer archivo del directorio*/
	hFind = FindFirstFile(szDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind) 
	{
		printf("Error.\r\n");
		printf("FindFirstFile: %d\r\n\\r\n", GetLastError());
		return -1;
	} 

	/*Recorre todos los archivos del directorio y sus subdirectorios y les aplica la funcion a cada uno*/
	do
	{
		if (!strcmp(ffd.cFileName, ".") || !strcmp(ffd.cFileName, ".."))
			continue;
		if (ffd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
		{
			char newDir[MAX_PATH];

			wsprintf(newDir, "%s\\%s", directorio, ffd.cFileName);
			if (forAllFiles(newDir, funcion) < 0)
				return -1;
		}
		else
		{
			if (funcion(ffd) < 0)
				return -1;
		}
	}
	while (FindNextFile(hFind, &ffd) != 0);

	if (GetLastError() != ERROR_NO_MORE_FILES)
	{
		printf("Error.\r\n");
		printf("FindFirstFile: %d\r\n\\r\n", GetLastError());
		return -1;
	}

	FindClose(hFind);
	return 0;
}

int rutinaTrabajoCrawler(WIN32_FIND_DATA filedata)
{
	DWORD attr = filedata.dwFileAttributes;
	struct nlist *np = hashLookup(filedata.cFileName);
	char *filename = filedata.cFileName;
	char type[MAX_FORMATO];
	char md5[MAX_PATH];
	crawler_URL paquete;
	int mode;

	hashMD5(filename, config.directorioFiles, md5);

	if (np != NULL && attr == FILE_ATTRIBUTE_READONLY)
	{
		if (hashClean(filename) < 0)
			return -1;
	}

	else if ((np == NULL && attr != FILE_ATTRIBUTE_READONLY) || 
			(np != NULL && attr != FILE_ATTRIBUTE_READONLY && strcmp(np->md5, md5)))
	{
		int i, cantPalabras = 0;

		if (hashInstall(filename, md5) < 0)
			return -1;
		if (getFileType(filename, type) == HTML)
		{
			if (np == NULL && attr != FILE_ATTRIBUTE_READONLY)
				mode = IRC_CRAWLER_ALTA_HTML;
			else
				mode = IRC_CRAWLER_MODIFICACION_HTML;

			if (parsearHtml(filename, &paquete) < 0)
				return -1;
		}
		else
		{
			if (np == NULL && attr != FILE_ATTRIBUTE_READONLY)
				mode = IRC_CRAWLER_ALTA_ARCHIVOS;
			else
				mode = IRC_CRAWLER_MODIFICACION_ARCHIVOS;

			if (generarPaqueteArchivos(filename, &paquete, &cantPalabras) < 0)
				return -1;
		}

		/*Se envia el paquete al Web Store*/
		if (enviarPaquete(config.ipWebStore, config.puertoWebStore, &paquete, mode) < 0)
			printf("Error en el envio de Paquete al Web Store para %s", filename);

		/*Libero las palabras del paquete que fueron cargadas dinamicamente*/
		for (i=0;i<cantPalabras;i++)
			free(paquete.palabras[i]);
		paquete.palabras = NULL;
	}

	_CrtDumpMemoryLeaks();

	return 0;
}

int enviarPaquete(in_addr_t nDireccion, in_port_t nPort, crawler_URL *paquete, int mode)
{
    SOCKET sockWebStore;
    SOCKADDR_IN dirServidor;
    char descriptorID[DESCRIPTORID_LEN];

    /*Se levanta conexion con el Web Store*/
    if ((sockWebStore = establecerConexionServidor(nDireccion, nPort, &dirServidor)) < 0)
        return -1;
    printf("Se establecio conexion con WebStore en %s.\r\n", inet_ntoa(dirServidor.sin_addr));

	/*Se envia el paquete al Web Store*/
    if (ircResponse_send(sockWebStore, descriptorID, (void*) paquete, sizeof(*paquete), mode) < 0)
    {
        closesocket(sockWebStore);
        return -1;
    }
    printf("Paquete enviado a WebStore en %s.\r\n\r\n", inet_ntoa(dirServidor.sin_addr));

    closesocket(sockWebStore);
    
    return 0;
}

int generarPaqueteArchivos(const char *filename, crawler_URL *paquete, int *cantPalabras)
{   
    int ntype;
	DWORD size;
    char type[5];
	char filedir[MAX_PATH];	

    ntype = getFileType(filename, type);
	if (ntype < 0) return -1;

	lstrcpy(paquete->formato, type);
	if (ntype == JPG || ntype == GIF || ntype == PNG || ntype == JPEG)
		lstrcpy(paquete->tipo, "1");
	else
		lstrcpy(paquete->tipo, "2");

	wsprintf(filedir, "%s\\%s", config.directorioFiles, filename);
	size = getFileSize(filedir);
	if (size < 0) return -1;
	wsprintf(paquete->length, "%ld", size);

	if (getKeywords(filename, &(paquete->palabras), cantPalabras) < 0)
		return -1;
	wsprintf(paquete->URL, "http://%s:%d/%s", inet_ntoa(*(IN_ADDR *)&config.ip), ntohs(config.puerto), filename);

    printf("Size: %s\r\n", paquete->length);
	printf("Type: %s\r\n", paquete->tipo);
	printf("URL: %s\r\n", paquete->URL);
	printf("Formato: %s\r\n", paquete->formato);
	printf("\r\n");

	return 0;
}

int EnviarCrawler(in_addr_t nDireccion, in_port_t nPort)
{
    SOCKET sockWebServer;
    SOCKADDR_IN dirServidorWeb;
    char descriptorID[DESCRIPTORID_LEN];
    char buf[BUF_SIZE];
    int mode = 0x00;

    /*Se levanta conexion con el Web Server*/
    if ((sockWebServer = establecerConexionServidor(nDireccion, nPort, &dirServidorWeb)) < 0)
        return -1;
    printf("Se establecio conexion con WebServer en %s.\n", inet_ntoa(dirServidorWeb.sin_addr));

    /*Se envia mensaje de instanciacion de un Crawler*/
    if (ircRequest_send(sockWebServer, NULL, 0, descriptorID, IRC_CRAWLER_CONNECT) < 0)
    {
        closesocket(sockWebServer);
        return -1;
    }
    printf("Crawler disparado a %s.\n\n", inet_ntoa(dirServidorWeb.sin_addr));

    if (ircRequest_recv (sockWebServer, buf, descriptorID, &mode) < 0)
       return -1;

    if (mode == IRC_CRAWLER_OK)
       printf("Crawler a migrado safisfactoriamente a %s.\n\n", inet_ntoa(dirServidorWeb.sin_addr));
    else if (mode == IRC_CRAWLER_FAIL)
       printf("Crawler a sido rechazado de %s.\n\n", inet_ntoa(dirServidorWeb.sin_addr));
    else
       printf("Error en mensaje IRC, se descarta.\n\n");

    closesocket(sockWebServer);
    
    return 0;
}

/*ANTIGUA rutinaAtencionCrawler*/

/*void rutinaAtencionCrawler (LPVOID args)
{
	SOCKET sockWServ;
	SOCKADDR_IN dirWServ;
	
/*	char descriptorID[DESCRIPTORID_LEN];
	char *buf = malloc(sizeof(char));
	int mode = IRC_CRAWLER_CONNECT;
	int rtaLen;
	int pvez = (crawPresence == -1);
	
	_CrtDumpMemoryLeaks();

	/*Conexion con Web Server*/
/*	if ((sockWServ = establecerConexionServidor(config.ip, config.puertoCrawler, &dirWServ)) < 0)
	{
		printf("Error de conexion entre Crawler y Web Server. Se descarta Crawler.\r\n");
	}

	/*Envio de Handshake*/
/*	else if (ircRequest_send(sockWServ, (char *) IRC_CRAWLER_HANDSHAKE_CONNECT, 
						sizeof(IRC_CRAWLER_HANDSHAKE_CONNECT), descriptorID, mode) < 0)
	{
		printf("Error en mensaje ircRequest_send entre Crawler y Web Server. Se descarta Crawler.\r\n");
	}
	
	/*Recibe respuesta al Handshake*/
/*	else 
	{
		mode = 0x00;

		if (ircResponse_recv(sockWServ, &buf, &rtaLen, descriptorID, &mode) < 0)
		printf("Error en mensaje ircRequest_send entre Crawler y Web Server. Se descarta Crawler.\r\n");

		/*Se analiza respuesta*/
/*		else if (strcmp(buf, IRC_CRAWLER_HANDSHAKE_FAIL) == 0)

			/*Se descarta Crawler. Tiene prohibida la migracion.*/
/*			printf("Crawler rechazado por el WebServer. Se descarta Crawler.\r\n");

		else if (strcmp(buf, IRC_CRAWLER_HANDSHAKE_OK) == 0)
		{
			/*Mutua exclusion para la variable global de presencia de Crawler*/
/*			WaitForSingleObject(crawMutex, INFINITE);
			crawPresence = 1;
			ReleaseMutex(crawMutex);
			
			/*Procesar los archivos del directorio*/
/*			if (forAllFiles(config.directorioFiles, rutinaTrabajoCrawler) < 0)
				printf("Crawler: Error al procesar archivos. Se descarta Crawler.\r\n\r\n");	
		}
		else
			printf("Error en el mensajeo entre Crawler y Web Server. Mensajes invalidos");
	}
	
	printf("Analisis del Web Crawler a Finalizado.\r\n\r\n");

	free(buf);
	WaitForSingleObject(crawMutex, INFINITE);
	crawTimeStamp = GetTickCount();
	crawPresence = 0;
	ReleaseMutex(crawMutex);
	_endthreadex(0);
}

*/
