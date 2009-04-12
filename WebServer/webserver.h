#ifndef WEBSERVER
#define WEBSERVER

#include <stdio.h>
#include <process.h>
#include <winsock2.h>
#include <windows.h>

#define WIN32_LEAN_AND_MEAN
#define MAXCONEXIONES SOMAXCONN
#define BUF_SIZE 4096
#define MAX_HEAP 1024*1024

#define STR_MSG_HELP "USO:\n\t-help: Desplega modo de uso\n\t-queuestatus: Desplega estado de la Cola de Espera\n\t-run: Pone el Web Server en funcionamiento\n\t-finish: Finaliza el Web Server\n\t-outofservice: Establece al Web Server fuera de servicio\n\n"
#define STR_MSG_QUEUESTATUS "Estado de la Cola de Espera:\n\n"
#define STR_MSG_RUN "Web Server en servicio\n\n"
#define STR_MSG_INVALID_RUN "Web Server ya se encuentra en servicio\n\n"
#define STR_MSG_FINISH "Web Server a finalizado\n"
#define STR_MSG_OUTOFSERVICE "Web Server fuera de servicio\n\n"
#define STR_MSG_INVALID_OUTOFSERVICE "Web Server ya se encuentra fuera de servicio\n\n"
#define STR_MSG_INVALID_INPUT "Comando invalido. Escriba -help para informacion de comandos\n\n"

enum codop_t {RUNNING, FINISH, OUTOFSERVICE};

typedef unsigned int in_addr_t;
typedef unsigned short in_port_t;

#endif