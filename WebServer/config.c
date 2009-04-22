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
Autor: Scheinkman, Mariano
Recibe: estructura configuracion.
Devuelve: ok? 0: -1
 */

int leerArchivoConfiguracion(configuracion *config)
{
	/*HACER*/
	config->ip = INADDR_ANY;
	config->puerto = 4444;
	config->cantidadClientes = 2;
	config->archivoLog = "c:\\webserver";
	lstrcpy(config->directorioFiles, "D:\\Downloads");
	config->esperaMaxima = 2000;

	return 0;
}