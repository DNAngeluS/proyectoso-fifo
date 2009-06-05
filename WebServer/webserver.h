#ifndef WEBSERVER
#define WEBSERVER

#include <stdio.h>
#include <time.h>
#include <conio.h>
#include <string.h>
#include <process.h>
#include <winsock2.h>
#include <windows.h>
#include "http.h"

#define WIN32_LEAN_AND_MEAN
#define BUF_SIZE 1024
#define MAX_HEAP 1024*1024
#define MAX_INPUT_CONSOLA 30

enum estado_t {ESPERA, ATENDIDO};

enum codop_t {RUNNING, FINISH, OUTOFSERVICE};

typedef unsigned int in_addr_t;
typedef unsigned short in_port_t;

/*Definiciones para el manejo de los mensajes de consola*/
#define STR_MSG_HELP "USO:\r\n\t-help: Desplega modo de uso\r\n\t-queuestatus: Desplega estado de la Cola de Espera\r\n\t-run: Pone el Web Server en funcionamiento\r\n\t-finish: Finaliza el Web Server\r\n\t-outofservice: Establece al Web Server fuera de servicio\r\n\t-private file: Establece a file como archivo privado\r\n\t-public file: Establece a file como archivo publico\r\n\t-files: Desplega la lista de archivos y su hash\r\n\t-reset: Limpia la Tabla Hash de archivo publicos del Web Server\r\n\r\n"
#define STR_MSG_QUEUESTATUS "Estado actual de la Cola de Espera:\r\n\r\n"
#define STR_MSG_HASH "Estado actual de la Tabla Hash para archivos publicos es:\r\n\r\n"
#define STR_MSG_RUN "Web Server en servicio\r\n\r\n"
#define STR_MSG_RESET "Se a limpiado la Tabla Hash\r\n\r\n"
#define STR_MSG_FINISH "Web Server a finalizado\r\n\r\n"
#define STR_MSG_OUTOFSERVICE "Web Server fuera de servicio\r\n\r\n"
#define STR_MSG_INVALID_OUTOFSERVICE "Error: Web Server ya se encuentra fuera de servicio. Ingrese -run para activar el servicio\r\n\r\n"
#define STR_MSG_INVALID_INPUT "Error: Comando invalido. Escriba -help para informacion de comandos\r\n\r\n"
#define STR_MSG_WELCOME "----Web Server----\r\n------------------\r\n\r\n"
#define STR_MSG_INVALID_PRIVATE "Error: El archivo ya es privado\r\n\r\n"
#define STR_MSG_INVALID_PUBLIC "Error: El archivo ya es publico\r\n\r\n"
#define STR_MSG_INVALID_ARG "Error: Argumento invalido\r\n\r\n"
#define STR_MSG_INVALID_FILE "Error: El archivo no existe\r\n\r\n"
#define STR_MSG_INVALID_RESET "No se pudo realizar la limpieza de la Tabla Hash. Se conserva su estado actual.\r\nIngrese -files para conocer el estado actual de la Tabla Hash\r\n\r\n"
#define STR_MSG_INVALID_RUN "Error: Web Server ya se encuentra en servicio. Ingrese -outofservice para desactivar servicio o -finish para finalizar\r\n\r\n"


#define ENCABEZADO_LOG "Archivo Log - Web Server\r\n\r\n"

/*Estructuras para la cola de threads*/
struct thread {
	DWORD threadID;
	SOCKET socket;
	SOCKADDR_IN direccion;
	HANDLE threadHandle;
	DWORD arrival;
	msgGet getInfo;
	DWORD bytesEnviados;
	BOOL estado;
};

typedef struct nodoListaThread{
	struct thread info;
	struct nodoListaThread *sgte;
} NodoListaThread;

typedef NodoListaThread *ptrListaThread;

/*Estructuras para el archivo Log*/
typedef struct {
	unsigned numRequests;
	DWORD numBytes;
	SYSTEMTIME arrivalKernel;
	SYSTEMTIME arrivalUser;
	DWORD arrival;
} infoLogFile;

#endif