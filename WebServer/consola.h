#ifndef CONSOLA
#define CONSOLA

#include "webserver.h"
#ifndef CONFIG
	#include "config.h"
#endif
#ifndef HASH
	#include "hash.h"
#endif
#ifndef THREADLIST
	#include "threadlist.h"
#endif

#define MAX_INPUT_CONSOLA 30

/*Definiciones para el manejo de los mensajes de consola*/
#define STR_MSG_HELP "USO:\r\n\t-help: Desplega modo de uso\r\n\t-queuestatus: Desplega estado de la Cola de Espera\r\n\t-run: Pone el Web Server en funcionamiento\r\n\t-finish: Finaliza el Web Server\r\n\t-outofservice: Establece al Web Server fuera de servicio\r\n\t-private file: Establece a file como archivo privado\r\n\t-public file: Establece a file como archivo publico\r\n\t-files: Desplega la lista de archivos y su hash\r\n\t-reset: Limpia la Tabla Hash de archivo publicos del Web Server\r\n\r\n"
#define STR_MSG_QUEUESTATUS "Estado actual de la Cola de Espera:\r\n\r\n"
#define STR_MSG_HASH "Estado de la Tabla Hash segun ultima migracion de Web Crawler:\r\n\r\n"
#define STR_MSG_RUN "Web Server en servicio\r\n\r\n"
#define STR_MSG_RESET "Se a limpiado la Tabla Hash\r\n\r\n"
#define STR_MSG_FINISH "Web Server a finalizado\r\n\r\n"
#define STR_MSG_OUTOFSERVICE "Web Server fuera de servicio\r\n\r\n"
#define STR_MSG_PUBLIC "El archivo es ahora publico. Proximos Crawlers podran detectarlos\r\n\r\n"
#define STR_MSG_PRIVATE "El archivo es ahora privado. Proximos Crawlers no podran detectarlos\r\n\r\n"
#define STR_MSG_INVALID_OUTOFSERVICE "Error: Web Server ya se encuentra fuera de servicio. Ingrese -run para activar el servicio\r\n\r\n"
#define STR_MSG_INVALID_INPUT "Error: Comando invalido. Escriba -help para informacion de comandos\r\n\r\n"
#define STR_MSG_WELCOME "----Web Server----\r\n------------------\r\n\r\n"
#define STR_MSG_INVALID_PRIVATE "Error: El archivo ya es privado\r\n\r\n"
#define STR_MSG_INVALID_PUBLIC "Error: El archivo ya es publico\r\n\r\n"
#define STR_MSG_INVALID_ARG "Error: Argumento invalido\r\n\r\n"
#define STR_MSG_INVALID_FILE "Error: El archivo no existe\r\n\r\n"
#define STR_MSG_INVALID_RESET "No se pudo realizar la limpieza de la Tabla Hash. Se conserva su estado actual.\r\nIngrese -files para conocer el estado actual de la Tabla Hash\r\n\r\n"
#define STR_MSG_INVALID_RUN "Error: Web Server ya se encuentra en servicio. Ingrese -outofservice para desactivar servicio o -finish para finalizar\r\n\r\n"


void rutinaAtencionConsola (LPVOID args);

#endif