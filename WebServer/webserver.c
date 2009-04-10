#include "webserver.h"

void rutinaDeError (char *error);
void rutinaAtencionConsola (void *args);
void rutinaAtencionCliente (void *sockCliente);	/* HACER Rutina de Atencion de Clientes*/
SOCKET establecerConexionEscucha (const char* direccionIP, in_port_t nPort);
generarLog(); /*HACER Genera archivo log con datos estadisticos obtenidos*/


int codop = RUNNING;	/*
						Codigo de Estado de Servidor.
						0 = running
						1 = finish
						2 = out of service
						*/

configuracion config;		/*Estructura con datos del archivo de configuracion accesible por todos los Threads*/
HANDLE LogFileMutex;		/*Semaforo de mutua exclusion MUTEX para escribir archivo Log*/



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
	DWORD nThreadClienteID;

	/*Inicializacion de los sockets*/
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		rutinaDeError("WSAStartup");

	/*Lectura de Archivo de Configuracion*/
	if (leerArchivoConfiguracion(&config) != 0)
		rutinaDeError("Lectura Archivo de configuracion");

	/*Creacion de Thread de Atencion de Consola*/
	hThreadConsola = (HANDLE) _beginthreadex (NULL, 1, (void*) rutinaAtencionConsola, NULL, 0, &uiThreadConsolaID);
	if (hThreadConsola == 0)
		rutinaDeError("Creacion de Thread");

	/*Se establece conexion a puerto de escucha*/
	if ((sockWebServer = establecerConexionEscucha(config.dirIP, config.puerto) == INVALID_SOCKET)
		rutinaDeError("Socket invalido");



/*------------------------Atencion de Clientes-------------------------*/

	while (codop != FINISH)
	{
		if (codop != OUTOFSERVICE)
		{
			int nAddrSize = sizeof(dirCliente);

			sockCliente = accept(ListeningSocket, (SOCKADDR_IN *)&dirCliente, &nAddrSize);
			
			if (sockRemoto != INVALID_SOCKET) 
			{
				printf ("Conexion aceptada de %s:%d.\n", inet_ntoa(dirCliente.sin_addr), ntohs(dirCliente.sin_port));

				/* 
				CREAR NUEVO THREAD Y PONERLO EN LA COLA 
				
				Con la cola sabemos cuantos threads hay en ejecucion, y con eso podemos
				controlar a los que van llegando.

				Ej COLA:  THREAD1   THREAD2  THREAD3	THREAD4  THREAD5
						  runing	runing	 runing		wait	 wait
				
				Ni bien termina THREAD1, se elimina de la cola y THREAD4 pasa a ejecucion.
				Antes de elminiar THREAD1 se guardan los datos estadisticos para el archivo Log.
				
				*/
			}
			else
				rutinaDeError("Accept");
		}
	}
/*--------------------Fin de Atencion de Clientes----------------------*/



	/*Cierra el handle que uso para el thread de Atencion de Consola*/
	CloseHandle(hThreadConsola);

	/*Generar archivo Log*/
	generarLog();

	return 0;
}




/*      
Descripcion: Establece la conexion del servidor web a la escucha en un puerto y direccion ip.
Autor: Scheinkman, Mariano
Recibe: Direccion ip y puerto a escuchar.
Devuelve: ok? Socket del servidor: socket invalido.
 */

SOCKET establecerConexionEscucha(const char* sDireccionIP, int nPort)
{
    in_addr_t nDireccionIP = inet_addr(sDireccionIP);
    SOCKET sockfd;
	
	if (nDireccionIP != INADDR_NONE) 
	{
		/*Obtiene un socket*/
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd != INVALID_SOCKET) 
		{
            SOCKADDR_IN addrServidorWeb; /*Address del Web server*/
            
			addrServidorWeb.sin_family = AF_INET;
            addrServidorWeb.sin_addr.s_addr = nDireccionIP;
            addrServidorWeb.sin_port = nPort;
			memset(&addrServidorWeb.sin_zero), '\0', 8);
			
			/*Liga socket al puerto y direccion*/
            if (bind (sockfd, (SOCKADDR_IN *)&addrServidorWeb, sizeof(SOCKADDR_IN)) != SOCKET_ERROR) 
			{
				/*Pone el puerto a la escucha de conexiones entrantes*/
                if (listen(sockfd, MAX_CONEXIONES) == -1)
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
	}
	_endthreadex(0);
	return 0;
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