#include "config.h"
#include "webserver.h"

void rutinaDeError (char *error);
void rutinaAtencionConsola (void *args);
void rutinaAtencionCliente (void *sockCliente);	/* HACER Rutina de Atencion de Clientes*/
SOCKET establecerConexionEscucha (in_addr_t direccionIP, in_port_t nPort);
generarLog(); /*HACER Genera archivo log con datos estadisticos obtenidos*/
int AgregarThread(ptrListaThread *ths, HANDLE idHandle, DWORD threadID);
void EliminarThread(DWORD id);
void imprimeLista(ptrListaThread ptr);

int codop = RUNNING;	/*
						Codigo de Estado de Servidor.
						0 = running
						1 = finish
						2 = out of service
						*/

configuracion config;				/*Estructura con datos del archivo de configuracion accesible por todos los Threads*/

infoLogFile infoLog;
HANDLE logFileMutex;				/*Semaforo de mutua exclusion MUTEX para escribir archivo Log*/

ptrListaThread listaThread = NULL;	/*Puntero de la lista de threads de usuarios en ejecucion y espera*/
HANDLE threadListMutex;				/*Semaforo de mutua exclusion MUTEX para modificar la Lista de Threads*/


/*      
Descripcion: Provee de archivos a clientes solicitantes en la red.
Autor: Scheinkman, Mariano
Recibe: -
Devuelve: ok? 0: 1
*/

int main() {
	
	WSADATA wsaData;				/*Version del socket*/
	SOCKET sockWebServer;			/*Socket del Web Server*/
	SOCKET sockCliente;				/*Socket del cliente remoto*/
	SOCKADDR_IN dirCliente;			/*Direccion de la conexion entrante*/
	HANDLE hThreadConsola;			/*Thread handle de la atencion de Consola*/
	DWORD nThreadConsolaID;			/*ID del thread de atencion de Consola*/

	
	/*Tiempo de inicio de ejecucion del Servidor*/
	GetLocalTime(&(infoLog.arrival)); 

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
	hThreadConsola = (HANDLE) _beginthreadex (NULL, 1, (void *) rutinaAtencionConsola, NULL, 0, &nThreadConsolaID);
	if (hThreadConsola == 0)
		rutinaDeError("Creacion de Thread");

	/*Se establece conexion a puerto de escucha*/
	if ((sockWebServer = establecerConexionEscucha(config.ip, config.puerto)) == INVALID_SOCKET)
		rutinaDeError("Socket invalido");

	printf("%s", STR_MSG_WELCOME);




/*------------------------Atencion de Clientes-------------------------*/

	/*Mientras no se indique la finalizacion*/
	while (codop != FINISH)
	{
		/*Si el servidor No esta fuera de servicio puede atender clientes*/
		if (codop != OUTOFSERVICE)
		{
			int nAddrSize = sizeof(dirCliente);

			/*Acepta la conexion entrante*/
			sockCliente = accept(sockWebServer, (SOCKADDR *) &dirCliente, &nAddrSize);
			
			if (sockCliente != INVALID_SOCKET) 
			{
				DWORD threadID;
				HANDLE threadHandle;
				
				printf ("Conexion aceptada de %s:%d.\n\n", inet_ntoa(dirCliente.sin_addr), ntohs(dirCliente.sin_port));

				/*Se crea el thread encargado de agregar cliente en la cola y atenderlo*/
				threadHandle = (HANDLE) _beginthreadex (NULL, 1, (void *) rutinaAtencionCliente, (void *) sockCliente, 0, &threadID);
				

				if (threadHandle == 0)
				{
					printf("Error al crear thread de comunicacion para usuario de %s:%d.\n\n", inet_ntoa(dirCliente.sin_addr),
																							 ntohs(dirCliente.sin_port));
					/*rutinaCerrarConexion(sockCliente);*/
				}
			}
			else
			{
				printf("Error en Accept: %d", GetLastError());
                continue;
            }
		}
		else
		{
			printf ("No se acepta conexion de %s:%d.\nSERVIDOR FUERA DE SERVICIO -> ingrese -run para activar el servicio\n\n", inet_ntoa(dirCliente.sin_addr), ntohs(dirCliente.sin_port));
		}
	}
/*--------------------Fin de Atencion de Clientes----------------------*/



	WSACleanup();

	/*Cierra el handle que uso para el thread de Atencion de Consola*/
	CloseHandle(hThreadConsola);

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
            char yes=1;
            
            /*Impide el error "addres already in use"*/
		    if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1)
			   rutinaDeError("Setsockopt");
            
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

void rutinaAtencionConsola (void *args)
{
	char consola[30];
	
	/*Mientras no se ingrese la finalizacion*/
	while (codop != FINISH)
	{
		/*
		ACTUAR DE ACUERDO A DISTINTOS INPUTS
		Ej:
			-help				te tira los inputs posibles			-> codop = RUNNING
			-queuestatus		te tira el estado de la cola		-> codop = RUNNING
			-run				vuelve al servicio el servidor		-> codop = RUNNING
			-finish				cierra el servidor					-> codop = FINISH
			-outofservice		deja al server fuera de servicio	-> codop = OUTOFSERVICE
		*/
		int centinela;
	    

/**------	Validaciones a la hora de ingresar comandos	------**/


		centinela = scanf("%s", consola);	

		if (centinela == 1)
			if (*consola == '-')
			{
				if (!lstrcmp(consola, "-queuestatus"))
				{
                     printf("%s", STR_MSG_QUEUESTATUS);
					 imprimeLista (listaThread);
                }
                else if (!lstrcmp(&consola[1], "run"))
                {
                     if (codop != RUNNING)
                     {
                        printf("%s", STR_MSG_RUN);
                        codop = RUNNING;
                     }
                     else
                         printf("%s", STR_MSG_INVALID_RUN);
                }
                else if (!lstrcmp(&consola[1], "finish"))
                {
                     printf("%s", STR_MSG_FINISH);
                     codop = FINISH;
                }
                else if (!lstrcmp(&consola[1], "outofservice"))
                {
                     if (codop != OUTOFSERVICE)
                     {
                        printf("%s", STR_MSG_OUTOFSERVICE);
                        codop = OUTOFSERVICE;
                     }
                     else
                         printf("%s", STR_MSG_INVALID_OUTOFSERVICE);
                }
                else if (!lstrcmp(&consola[1], "help"))
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

int AgregarThread(ptrListaThread *ths, HANDLE idHandle, DWORD threadID)
{      
	ptrListaThread nuevo, actual, anterior;
       
	if (idHandle == 0) 
	{
		printf("Error en _beginthreadex(): %d\n", GetLastError());
		return -1;
	}
       
	if ( (nuevo = HeapAlloc(GetProcessHeap(), 0, sizeof (struct thread))) == NULL)
		return -1;
	
	nuevo->info.threadHandle = idHandle;
	nuevo->info.threadID = threadID;
	nuevo->info.estado = ESPERA;
	GetSystemTime(&(nuevo->info.arrival));
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

void EliminarThread(DWORD id)
{
	ptrListaThread ptrAux = listaThread;
	ptrListaThread ptrAnt = NULL;

	/*Se busca el cliente en lista*/
	while (ptrAux->info.threadID != id && ptrAux != NULL)
	{
		ptrAnt = ptrAux;
		ptrAux = ptrAux->sgte;
	}

	if (ptrAnt != NULL)
		ptrAnt->sgte = ptrAux->sgte;
	else
		listaThread = NULL;
	if (ptrAux == NULL)
		rutinaDeError("Inconcistencia de Threads");
	
	CloseHandle(ptrAux->info.threadHandle);
    HeapFree (GetProcessHeap(), 0, ptrAux);

}


/*      
Descripcion: Thread que Atiende al cliente
Autor: Scheinkman, Mariano
Recibe: Socket del cliente
Devuelve: -
*/

void rutinaAtencionCliente (void *socket)
{
	int control;
	char buf[BUF_SIZE];
	ptrListaThread ptrAux = listaThread;
	ptrListaThread ptrThreadActual = NULL;
	unsigned cantClientesAtendidos = 0;
	SOCKET sockCliente = (SOCKET ) socket; 

	/*Mutua exclusion para agregar el thread a la lista de threads global*/
	WaitForSingleObject(threadListMutex, INFINITE);
	control = AgregarThread(&listaThread, GetCurrentThread(), GetCurrentThreadId());
	ReleaseMutex(threadListMutex);

	if (control < 0)
	{
		printf("Memoria insuficiente para nuevo Usuario\n");
		/*rutinaCerrarConexion(sockCliente);*/
		_endthreadex(1);
	}
	else
		infoLog.numRequests++;

	/*Se comprueba la cantidad de clientes atendidos y hace cuanto estan los que estan en espera, todo el tiempo*/
	do
	{
		/*Si llega al final, volver a empezar*/
		if (ptrAux == NULL)
		{
			ptrAux = listaThread;
			cantClientesAtendidos = 0;
			continue;
		}
		/*Si el thread esta atendido*/
		if (ptrAux->info.estado = ATENDIDO)
		{
			cantClientesAtendidos++;
			continue;
		}
		/*Si el thread esta en espera*/
		if (ptrAux->info.estado = ESPERA)
		{
			/*
			Comprobar que el (LOCALTIME - ptrAux->info.arrival) >= config.esperaMaxima.
			Si es asi, eliminar de la lista (usar mutua exclusion), actualizar cantidadClientes, 
			y enviar un request timeout y terminar thread con _endthreadex(0);
			*/
		}
		ptrAux = ptrAux->sgte;
		
		/*Condicion de salida: si hay menos clientes atendidos de los que se permiten 
		y a la vez no hay mas clientes que monitorear el estado en la lista */
	} while ((cantClientesAtendidos < config.cantidadClientes) && ptrAux == NULL);

	ptrThreadActual = listaThread;

/*----Listo para atender al cliente----*/

	/*Se busca el cliente en lista*/
	while (ptrThreadActual->info.threadID != GetCurrentThreadId() && ptrThreadActual != NULL)
		ptrThreadActual = ptrThreadActual->sgte;
	
	if (ptrThreadActual == NULL)
		rutinaDeError("Inconcistencia de Threads");

	/*Se actualiza estado de dicho cliente*/
	ptrThreadActual->info.estado = ATENDIDO;	
	
	/*MENSAJEO HTTP Y ENVIO DE ARCHIVO*/

	recv(sockCliente, buf, sizeof(buf), 0);
	printf("Mensaje: %s\n",buf);


	/*Cuando termina mensajeo se encarga de borrarse de la lista y finaliza Thread*/
	WaitForSingleObject(threadListMutex, INFINITE);
	
	EliminarThread(GetCurrentThreadId());

	ReleaseMutex(threadListMutex);

	_endthreadex(0);	
}

void imprimeLista(ptrListaThread ptr)
{
	int threadName = 65;
	
	if (ptr == NULL)
		printf("Lista esta vacia\n\n");
	else
	{
		printf("La lista es:\n");
         
		while (ptr != NULL)
		{
			printf("Request %c -> ", threadName);
			ptr = ptr->sgte;
			threadName++;
		}
		printf("NULL\n\n");
	}
}