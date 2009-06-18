#include "config.h"
#include "webserver.h"
#include "http.h"
#include "irc.h"
#include "hash.h"
#include "consola.h"
#include "connection.h"
#include "threadlist.h"
#include "crawler.h"

void rutinaDeError(char *error);
void logInicio();
int logFinal(infoLogFile infoLog);

configuracion config;
infoLogFile infoLog;
HANDLE logMutex;
HANDLE crawMutex;
HANDLE listMutex;
ptrListaThread listaThread;
int codop = RUNNING;
int crawPresence = -1;
DWORD crawTimeStamp = 0;
struct hashManager hashman;

int main()
{
	WSADATA wsaData;
	SOCKET sockWebServerCliente;
	SOCKET sockWebServerCrawler;

	HANDLE hThreadConsola;
	DWORD nThreadConsolaID;

	fd_set fdLectura;
	fd_set fdMaestro;

	/*Lectura de Archivo de Configuracion*/
	if (leerArchivoConfiguracion(&config) != 0)
		rutinaDeError("Lectura Archivo de configuracion");
	printf("Se a cargado el Archivo de Configuracion.\r\n");


	/*Inicializar Hash Table con datos anteriores, si existen*/
	if (hashLoad() < 0)
		printf("Error tratando de cargar Tabla Hash. Continua ejecucion.\r\n\r\n");
	
	/*Inicializo variables para reporte de ejecucion y escribe el encabezado*/
	logInicio();
	infoLog.arrival = GetTickCount();
	infoLog.numBytes = 0;
	infoLog.numRequests = 0;	

	/*Inicializacion de los Semaforos Mutex*/
	if ((logMutex = CreateMutex(NULL, FALSE, NULL)) == NULL)
		rutinaDeError("Error en CreateMutex()");
	if ((crawMutex = CreateMutex(NULL, FALSE, NULL)) == NULL)
		rutinaDeError("Error en CreateMutex()");
	if ((listMutex = CreateMutex(NULL, FALSE, NULL)) == NULL)
		rutinaDeError("Error en CreateMutex()");

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

	/*Se establece conexion a puerto de escucha de Clientes*/
	if ((sockWebServerCliente = establecerConexionEscucha(config.ip, config.puerto)) == INVALID_SOCKET)
		rutinaDeError("Socket invalido");

	printf("\r\n%s", STR_MSG_WELCOME);

	/*Se inicializan los FD_SET para select*/
	FD_ZERO(&fdMaestro);
	FD_SET(sockWebServerCliente, &fdMaestro);
	FD_SET(sockWebServerCrawler, &fdMaestro);

	/*Mientras no se indique la finalizacion*/
	while (codop != FINISH)
	{
		int numSockets, i;
		struct timeval timeout = {1, 0};

		FD_ZERO(&fdLectura);
		memcpy(&fdLectura, &fdMaestro, sizeof(fdMaestro));

		if ((numSockets = select(0, &fdLectura, 0, 0, &timeout)) < 0) 
			rutinaDeError("Select failed");

		for (i = 0; i < numSockets; i++)
		{
			/*== Hay usuario conectandose ==*/
			if (FD_ISSET(sockWebServerCliente, &fdLectura) && codop != FINISH)
			{
				SOCKET sockCliente = 0;

				if ((sockCliente = rutinaConexionCliente(sockWebServerCliente, config.cantidadClientes)) == -1 || sockCliente == -2)
				{
					printf("No se ha podido conectar cliente.%s\r\n", sockCliente == -2? 
						" Servidor esta fuera de servicio.\r\nIngrese -run para reanudar ejecucion.\r\n":"\r\n");
					
					if (sockCliente == -1)
					{
						/*No se pudo crear Thread. El timeout se ocupara de limpiarlo.*/
					}
				}
				/*else
					FD_SET(sockCliente, &fdMaestro);*/
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
				}
			}
		} /*End for (i = 0; i < numSockets; i++)*/

		/*== No hubo actividad -> Select() timeout ==*/
		if (numSockets == 0)
		{
			ptrListaThread ptrAux = NULL;

			/*Se recorre la lista buscando algun Request que ya este en timeout y se elimina*/
			for (ptrAux = listaThread; ptrAux != NULL; )
			{
				unsigned timeDiff = GetTickCount() - ptrAux->info.arrival;

				/*== Busqueda de usarios con Timeout ==*/
				if (ptrAux->info.estado == ESPERA && timeDiff > config.esperaMaxima)
				{
					printf("Resquet de %s timed-out.\r\n\r\n", inet_ntoa(ptrAux->info.direccion.sin_addr));
					if (httpTimeout_send(ptrAux->info.socket, ptrAux->info.getInfo) < 0)
						printf("Error %d al enviar Http timeout a %s.\r\n\r\n", GetLastError(), inet_ntoa(ptrAux->info.direccion.sin_addr));
					
					WaitForSingleObject(listMutex, INFINITE);
					ptrAux->info.estado = ATENDIDO;
					ReleaseMutex(listMutex);

					ptrAux->info.estado = ATENDIDO;
					closesocket(ptrAux->info.socket);
				}

				/*== Usuarios finalizados ==*/
				else if (ptrAux->info.estado == ATENDIDO)
				{
					ptrListaThread ptrSgte = ptrAux->sgte;
					
					WaitForSingleObject(ptrAux->info.threadHandle, INFINITE);
					EliminarThread(ptrAux->info.socket);
					
					ptrAux = ptrSgte;

					if (cantidadThreadsLista(listaThread, ATENDIENDOSE) <= config.cantidadClientes)
					{
						ptrListaThread ptr = BuscarProximoThread(listaThread);
						if (ptr != NULL)
						{
							WaitForSingleObject(listMutex, INFINITE);
							ptr->info.estado = ATENDIENDOSE;
							ReleaseMutex(listMutex);
							
							if (rutinaCrearThread(rutinaAtencionCliente, ptr->info.socket) != 0)
							{
								printf("No se pudo crear thread de Atencion de Cliente\r\n\r\n");
								closesocket(ptr->info.socket);
							}
						}
					}
					continue;
				}
				ptrAux = ptrAux->sgte;
			}
		} /*End if (numSockets == 0)*/
	} /*End while (codop != FINISH)*/

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

void rutinaDeError (char *error) 
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