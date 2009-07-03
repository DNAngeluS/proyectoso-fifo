/* 
==================
config.c 

Descripcion: Biblioteca de lectura y manejo de archivo de configuracion
Ultima modificacion: 10/4/09 16:10
Modificado por: Scheinkman, Mariano
==================
*/

#include "config.h"

#define BUF_SIZE 4096   /* Maximo tamaño del archivo de configuracion */


/*      
Descripcion: Carga en estructura de configuracion valores del archivo de configuracion
Ultima Modificacion: Scheinkman, Mariano
Recibe: estructura configuracion.
Devuelve: ok? 0: -1
 */
int leerArchivoConfiguracion(configuracion *config)
{
	char buf[BUF_SIZE];
	char *key, *value, *act, *primero, *lim;
	int buf_size, temp;
	HANDLE archivoConfig;

	archivoConfig = CreateFileA("config.cfg", GENERIC_READ, 0, NULL,
						  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (archivoConfig == INVALID_HANDLE_VALUE) {
		 printf("No se pudo abrir el archivo de configuracion: "
								 "(error %d)\r\n", GetLastError());
		 return 1;
	}
	temp = ReadFile(archivoConfig, buf, BUF_SIZE, &buf_size, NULL);
	CloseHandle(archivoConfig);
	if (temp == FALSE) {
		 printf("No se pudo leer el archivo de configuracion: "
								 "(error %d)\r\n", GetLastError());
		 return 1;
	}

	key = value = NULL;
	lim = buf + buf_size;
	for (primero = act = buf; act < lim; act++) {
		 switch (*act) {
		 case '=':
				 *act = '\0';
				 key = primero;
				 value = act + 1;
				 break;
		 case '\r': 
				 /* En windows, nueva linea es "\r\n" */
				 if (*(act+1) != '\n') {
					printf ("Error en la sintaxis del archivo de " 
										 "configuracion: \\r sin \\n");
					return 1;
				 }
				 *(act++) = '\0';
				 primero = act + 1;
		 case ' ':
		 case '\t':
		 if (key != NULL && value != NULL) {
				 *act = '\0';
				 if (asignarDatos(config, key ,value) < 0)
						 return -1;
		 }
		 /* En cualquier caso, reseteamos los punteros key y value */
		 case '#':
				 key = value = NULL;
				 break;
		 }
	}

	return 0;
}

/*      
Descripcion: Asigna datos en estructura segun el key y value
Ultima Modificacion: Scheinkman, Mariano
Recibe: estructura configuracion, key, value.
Devuelve: ok? 0: -1
 */
int asignarDatos (configuracion *config, char *key, char *value)
{
	if (!strcmp(key, "IP")) 
		config->ip = inet_addr(value);

	else if (!strcmp(key, "PUERTO"))
		config->puerto = htons(atoi(value));

	else if (!strcmp(key, "IP_WEBSTORE")) 
		config->ipWebStore = inet_addr(value);

	else if (!strcmp(key, "PUERTO_WEBSTORE"))
		config->puertoWebStore = htons(atoi(value));

	else if (!strcmp(key, "PATH_DOWNLOADS"))
		strcpy_s(config->directorioFiles, MAX_PATH, value);

	else if (!strcmp(key, "PATH_HASH"))
		strcpy_s(config->directorioHash, MAX_PATH, value);

	else if (!strcmp(key, "MAX_CLIENTES_CONCURRENTES"))
		config->cantidadClientes = (unsigned) atoi(value);

	else if (!strcmp(key, "MAX_ESPERA_CLIENTES"))
		config->esperaMaxima = (unsigned) atoi(value);

	else if (!strcmp(key, "PUERTO_CRAWLER"))
		config->puertoCrawler = htons(atoi(value));

	else if (!strcmp(key, "MAX_ESPERA_CRAWLER"))
		config->esperaCrawler = (unsigned) atoi(value);

	else if (!strcmp(key, "PATH_LOG"))
	{
		config->archivoLog = CreateFileA(value, GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
		
		if (config->archivoLog == INVALID_HANDLE_VALUE) {
			printf("No se pudo abrir el archivo log\r\n", GetLastError());
			return -1;
		}
		SetFilePointer(config->archivoLog, 0, NULL, FILE_END);
	}

	else 
	{
		printf("Error en la sintaxis del archivo de " 
		"configuracion: key '%s' invalida\r\n", key);
		return -1;
	}

	return 0;
}