#ifndef WEBSERVER
#define WEBSERVER

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <stdio.h>
#include <time.h>
#include <conio.h>
#include <string.h>
#include <process.h>
#include <winsock2.h>
#include <windows.h>
#ifndef HTTP
	#include "http.h"
#endif

/*ESTO ES PARA DEBUGUEAR MEMORY LEAKS*/
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
/*ESTO ES PARA DEBUGUEAR MEMORY LEAKS*/
/*Instruccion para ver leaks -->  _CrtDumpMemoryLeaks();   */

/*#define BUF_SIZE 2048*/
#define MAX_HEAP 1024*1024

enum codop_t {RUNNING, FINISH, OUTOFSERVICE};
enum estado_t {ESPERA, ATENDIENDOSE, ATENDIDO};
enum proceso_t {ELIMINA_NO_PUBLICO, ATIENDE_NUEVO, ATIENDE_MODIFICADO, ARCHIVO_NO_SUFRIO_CAMBIOS};

typedef unsigned int in_addr_t;
typedef unsigned short in_port_t;

#define ENCABEZADO_LOG "Archivo Log - Web Server\r\n\r\n"
#define FINAL_LOG "---Web Server Finalizado---\r\n\r\n"

struct clienteLogData {
	in_addr_t ip;
	/*User Agent ¿?*/
};

/*Estructuras para el archivo Log*/
typedef struct {
	unsigned numRequests;				/*Numero de Clientes aceptados*/
	struct clienteLogData *clientes;	/*El IP y el User Agent de cada Cliente*/
	
	DWORD numBytes;						/*Numero Total de Bytes Transferidos*/
	DWORD kernelModeTotal;				/*Total de tiempo de ejecucion en modo Kernel*/
	DWORD userModeTotal;				/*Total de tiempo de ejecucion en mode Usuario*/

	DWORD arrival;						/*Momento de inicio de ejecucion del Web Server*/
} infoLogFile;

#endif