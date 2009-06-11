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
	in_addr_t ipWebStore;				/*ip del Web Store*/
	in_port_t puertoWebStore;			/*puerto del Web Store*/
	unsigned cantidadClientes;			/*cantidad de atenciones simultaneas*/
	unsigned esperaMaxima;				/*tiempo de espera maxima*/
	in_port_t puertoCrawler;			/*puerto local para escuchar Crawlers*/
	unsigned esperaCrawler;				/*tiempo de espera de un crawler*/
} configuracion;

int leerArchivoConfiguracion (configuracion *config);
int asignarDatos (configuracion *config, char *key, char *value);

#endif