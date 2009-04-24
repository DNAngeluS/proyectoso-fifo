#ifndef CONFIG
#define CONFIG

#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

typedef unsigned int in_addr_t;
typedef unsigned short in_port_t;

typedef struct {
    char directorioFiles[MAX_PATH];		/*directorio de archivos*/
    HANDLE archivoLog;					/*nombre del archivo log*/
    in_addr_t ip;						/*ip local*/
	in_port_t puerto;					/*puerto local*/
	unsigned cantidadClientes;			/*cantidad de atenciones simultaneas*/
	unsigned esperaMaxima;				/*tiempo de espera maxima*/
} configuracion;

int leerArchivoConfiguracion (configuracion *config);
int asignarDatos (configuracion *config, char *key, char *value);

#endif