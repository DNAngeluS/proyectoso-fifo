#ifndef CONFIG
#define CONFIG

typedef unsigned int in_addr_t;
typedef unsigned short in_port_t;

typedef struct {
        char directorioFiles[MAX_PATH];
        HANDLE archivoLog;
        in_addr_t ip;
        in_port_t puerto;
} configuracion;

int leerArchivoConfiguracion(configuracion *config);

#endif