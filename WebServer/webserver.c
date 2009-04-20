#include "config.h"
#include "webserver.h"
#include "http.h"

void rutinaDeError (char *error);
void rutinaAtencionConsola (LPVOID args);
void rutinaAtencionCliente (LPVOID args);

SOCKET rutinaConexionCliente(SOCKET sockWebServer, unsigned maxClientes);
SOCKET establecerConexionEscucha (in_addr_t direccionIP, in_port_t nPort);

void imprimeLista(ptrListaThread ptr);
int listaVacia(ptrListaThread listaThread);
int AgregarThread(ptrListaThread *ths, SOCKET socket, SOCKADDR_IN dirCliente);
void EliminarThread(SOCKET cli);
unsigned cantidadThreadsLista(ptrListaThread listaThread);

int rutinaCrearThread(void (*funcion)(LPVOID), SOCKET sockfd);
ptrListaThread BuscarThread(ptrListaThread listaThread, SOCKET sockfd);
ptrListaThread BuscarProximoThread(ptrListaThread listaThread);

generarLog(); /*HACER Genera archivo log con datos estadisticos obtenidos*/




configuracion config;				/*Estructura con datos del archivo de configuracion accesible por todos los Threads*/

infoLogFile infoLog;
HANDLE logFileMutex;				/*Semaforo de mutua exclusion MUTEX para escribir archivo Log*/

HANDLE threadListMutex;				/*Semaforo de mutua exclusion MUTEX para modificar la Lista de Threads*/

int codop = RUNNING;	/*
							Codigo de Estado de Servidor.
							0 = running
							1 = finish
							2 = out of service
						*/

ptrListaThread listaThread = NULL;	/*Puntero de la lista de threads de usuarios en ejecucion y espera*/

/*      
Descripcion: Provee de archivos a clientes solicitantes en la red.
Autor: Scheinkman, Mariano
Recibe: -
Devuelve: ok? 0: 1
*/

int main() 
{
	
	WSADATA wsaData;					/*Version del socket*/
	SOCKET sockWebServer;				/*Socket del Web Server*/

	HANDLE hThreadConsola;				/*Thread handle de la atencion de Consola*/
	DWORD nThreadConsolaID;				/*ID del thread de atencion de Consola*/

	fd_set fdLectura;
	fd_set fdMaestro;

	int fueradeservicio = 0;

	/*Inicializo variable para archivo Log*/
	GetLocalTime(&(infoLog.arrival)); 
	infoLog.numBytes = 0;
	infoLog.numRequests = 0;

	/*Inicializacion de los Semaforos Mutex*/
	if ((logFileMutex = CreateMutex(NULL, FALSE, NULL)) == NULL)
		rutinaDeError("Error en CreateMutex()");
	if ((threadListMutex = CreateMutex(NULL, FALSE, NULL)) == NULL)
		rutinaDeError("Error en CreateMutex()");
	
	/*Inicializacion de los sockets*/
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		rutinaDeError("WSAStartup");

	/*Lectura de Archivo de Configuracion*/
	if (leerArchivoConfiguracion(&config) != 0)
		rutinaDeError("Lectura Archivo de configuracion");

	/*Creacion de Thread de Atencion de Consola*/
	hThreadConsola = (HANDLE) _beginthreadex (NULL, 1, (LPVOID) rutinaAtencionConsola, NULL, 0, &nThreadConsolaID);
	if (hThreadConsola == 0)
		rutinaDeError("Creacion de Thread");

	/*Se establece conexion a puerto de escucha*/
	if ((sockWebServer = establecerConexionEscucha(config.ip, config.puerto)) == INVALID_SOCKET)
		rutinaDeError("Socket invalido");

	printf("%s", STR_MSG_WELCOME);

	FD_ZERO(&fdMaestro);
	FD_SET(sockWebServer, &fdMaestro);

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
				WaitForSingleObject(threadListMutex, INFINITE);
				ptrAux = listaThread;
				ReleaseMutex(threadListMutex);

				while (ptrAux != NULL)
				{
					if (ptrAux->info.estado == ESPERA && 
						(unsigned) difftime(time(NULL), ptrAux->info.arrival) > (config.esperaMaxima/1000))
					{
						ptrListaThread ptrSave = ptrAux->sgte;
						SOCKET sock = ptrAux->info.socket;
									
						printf("Resquet de cliente timed out. Se envia Http Request Timeout\n\n");
						/*ENVIAR HTTP REQUEST TIMEOUT a ptrAux->info.socket*/
						EliminarThread(ptrAux->info.socket);
						FD_CLR(sock, &fdMaestro);
						ptrAux = ptrSave;
					}
					else
						ptrAux = ptrAux->sgte;
				}
			continue;
		}


		/*== Hay usuario conectandose ==*/
		if (FD_ISSET(sockWebServer, &fdLectura))
		{
			SOCKET sockCliente;

			if ((sockCliente = rutinaConexionCliente(sockWebServer, config.cantidadClientes)) == -1 || sockCliente == -2)
			{
				printf("No se ha podido conectar cliente.%s\n", sockCliente == -2? 
					" Servidor esta fuera de servicio.\nIngrese -run para reanudar ejecucion.\n":"\n");
				
				if (sockCliente == -1)
					EliminarThread(sockCliente);
			}
			else
			{
				FD_SET(sockCliente, &fdMaestro);
				infoLog.numRequests++;
			}	
		}

		/*== Hay usuario desconectandose ==*/
		else
		{
			DWORD bytesTransferidos;
			ptrListaThread ptrAux = listaThread;
			SOCKET cli;

			while (ptrAux != NULL && !FD_ISSET((cli = ptrAux->info.socket), &fdLectura))
				ptrAux = ptrAux->sgte;
			if (ptrAux == NULL)
				rutinaDeError("Inconcistencia en lista o usuario no desconectado mal detectado");
			GetExitCodeThread(ptrAux->info.threadHandle, &bytesTransferidos);

			infoLog.numBytes = infoLog.numBytes + (DWORD) bytesTransferidos;

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
						printf("No se pudo crear thread de Atencion de Cliente\n\n");
						return -1;
					}
				}
			}
		}
	}
/*--------------------Fin de Atencion de Clientes----------------------*/



	WSACleanup();


	/*Generar archivo Log*/
	/*generarLog();*/

	return 0;
}




/*      
Descripcion: Establece la conexion del servidor web a la escucha en un puerto y direccion ip.
Autor: Scheinkman, Mariano
Recibe: Direccion ip y puerto a escuchar.
Devuelve: ok? Socket del servidor: socket invalido.
*/

SOCKET establecerConexionEscucha(in_addr_t nDireccionIP, in_port_t nPort)
{
    SOCKET sockfd;
	
	if (nDireccionIP != INADDR_NONE) 
	{
		/*Obtiene un socket*/
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd != INVALID_SOCKET) 
		{
			SOCKADDR_IN addrServidorWeb; /*Address del Web server*/
			char yes = 1;
			int NonBlock = 1;

			/*Impide el error "addres already in use" y setea non blocking el socket*/
			if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1)
				rutinaDeError("Setsockopt");

			/*if (ioctlsocket(sockfd, FIONBIO, &NonBlock) == SOCKET_ERROR)
			{
				rutinaDeError("ioctlsocket");
			}*/
            
			addrServidorWeb.sin_family = AF_INET;
            addrServidorWeb.sin_addr.s_addr = nDireccionIP;
            addrServidorWeb.sin_port = htons(nPort);
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
    }
	else
		rutinaDeError("Direccion IP invalida\n");
    return INVALID_SOCKET;
}



/*      
Descripcion: Thread que atiende los ingresos de usuario por consola.
Autor: Scheinkman, Mariano
Recibe: -
Devuelve: 0
*/

void rutinaAtencionConsola (LPVOID args)
{
	/*Mientras no se ingrese la finalizacion*/
	while (codop != FINISH)
	{
		char consolaStdin[30];
		int centinela=0;	    

/**------	Validaciones a la hora de ingresar comandos	------**/
		
		centinela = scanf("%s", consolaStdin);	

		if (centinela == 1)
			if (*consolaStdin == '-')
			{
				if (!lstrcmp(consolaStdin, "-queuestatus"))
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
Autor: Scheinkman, Mariano
Recibe: Error detectado.
Devuelve: -
*/

void rutinaDeError (char* error) 
{
	printf("\nError: %s\n", error);
	printf("Codigo de Error: %d\n\n", GetLastError());
	exit(1);
}


/*      
Descripcion: Agrega Thread a la lista de threads, encolandolos.
Autor: Scheinkman, Mariano
Recibe: Puntero a la lista de threads, el handle del nuevo thread y su ID.
Devuelve: ok? 0: -1
*/

int AgregarThread(ptrListaThread *ths, SOCKET socket, SOCKADDR_IN dirCliente)
{      
	ptrListaThread nuevo, actual, anterior;
       
	if (socket == INVALID_SOCKET) 
	{
		printf("Error en _beginthreadex(): %d\n", GetLastError());
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
	nuevo->info.arrival = time(NULL);
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
	
	if (ptrAux->info.threadHandle != 0)
		WaitForSingleObject(ptrAux->info.threadHandle, INFINITE);

	closesocket(ptrAux->info.socket);
	if (ptrAux->info.threadHandle != 0)
		CloseHandle(ptrAux->info.threadHandle);
	ptrAux->sgte = NULL;
	HeapFree (GetProcessHeap(), 0, ptrAux);
}


/*      
Descripcion: Thread que Atiende al cliente
Autor: Scheinkman, Mariano
Recibe: Socket del cliente
Devuelve: -
*/

void rutinaAtencionCliente (LPVOID args)
{
	char buf[BUF_SIZE];
	int timedout = 0;
	msgGet getInfo;
	DWORD bytesEnviados = 0, recibidos = 0;

	SOCKET sockCliente = (SOCKET) args;

	/*----Listo para atender al cliente----*/	
	
	/*RECIBIR HTTP GET, BUSCAR ARCHIVO Y ENVIAR RESPUESTA Y ARCHIVO*/

	httpGet_recv(sockCliente, &getInfo);
	printf("Filename: %s. Protocolo: %d \n", getInfo.filename, getInfo.protocolo);

	_endthreadex(bytesEnviados);	
}

void imprimeLista(ptrListaThread ptr)
{	
	if (ptr == NULL)
		printf("La cola esta vacia\n\n");
	else
	{
		printf("La lista es:\n");
         
		while (ptr != NULL)
		{
			printf("Request %d -> ", ptr->info.socket);
			ptr = ptr->sgte;
		}
		printf("NULL\n\n");
	}
}

int rutinaCrearThread(void (*funcion)(LPVOID), SOCKET sockfd)
{
	HANDLE threadHandle;
	DWORD threadID;
	ptrListaThread ptr = NULL;

	ptr = BuscarThread(listaThread, sockfd);
	if (ptr == NULL)
		rutinaDeError("Inconcistencia en la Lista");

	printf ("Se comienza a atender Request de %s:%d.\n\n", inet_ntoa(ptr->info.direccion.sin_addr), ntohs(ptr->info.direccion.sin_port));

	/*Se crea el thread encargado de atender cliente*/
	threadHandle = (HANDLE) _beginthreadex (NULL, 1, (void *) rutinaAtencionCliente, (LPVOID) sockfd, 0, &threadID);
	
	/*Se actualiza el nodo*/
	ptr->info.threadHandle = threadHandle;
	ptr->info.threadID = threadID;

		
	if (threadHandle == 0)
		return -1;
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
			int control;

			printf ("Conexion aceptada de %s:%d.\n\n", inet_ntoa(dirCliente.sin_addr), ntohs(dirCliente.sin_port));

			/*Mutua exclusion para agregar el thread a la lista de threads global*/
			control = AgregarThread(&listaThread, sockCliente, dirCliente);

			if (control < 0)
			{
				printf("Memoria insuficiente para nuevo Usuario\n\n");
				return -1;
			}

			if (cantidadThreadsLista(listaThread) <= maxClientes)
			{
				ptrListaThread ptr = BuscarThread(listaThread, sockCliente);
				if (ptr != NULL)
				{
					ptr->info.estado = ATENDIDO;
					if (rutinaCrearThread(rutinaAtencionCliente, sockCliente) != 0)
					{
						printf("No se pudo crear thread de Atencion de Cliente\n\n");
						return -1;
					}
				}
				else
					rutinaDeError("Inconcistencia en Lista");
			}
			return sockCliente;
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

/*{
	arrival = time(NULL);

	do
		{		
		/* Si el tiempo desde que llego excede lo configurado o se detecta que se puede seguir */
/*			if ((unsigned) difftime(time(NULL), arrival) > (config.esperaMaxima/1000))
			{
				/*ENVIAR MENSAJE REQUEST TIMEOUT*/
/*				printf("Thread %l timed out.\n\n", GetCurrentThreadId());
				_endthreadex(0);
			}

		/*Si llega al final de la cola, volver a empezar*/
/*		if (ptrAux == NULL)
		{
			ptrAux = listaThread;
			cantClientesAtendidos = 0;
			continue;
		}

		/*Si el thread esta atendido seguir buscando*/
/*		if (ptrAux->info.estado == ATENDIDO)
			cantClientesAtendidos++;

		ptrAux = ptrAux->sgte;

		/*Condicion de salida: si hay menos clientes atendidos de los que se permiten 
					y a la vez no hay mas clientes que monitorear el estado en la lista */
/*		} while ((cantClientesAtendidos < config.cantidadClientes) && ptrAux == NULL);
}*/

