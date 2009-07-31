/* 
 * File:   config.h
 * Author: marianoyfer
 *
 * Created on 22 de mayo de 2009, 11:18
 */

#ifndef _CONFIG_H
#define	_CONFIG_H

#define PUERTO_CRAWLER 4445

typedef unsigned int in_addr_t;
typedef unsigned short in_port_t;

typedef struct {
    in_addr_t ipWebServer;
    in_port_t puertoWebServer;
    in_port_t puertoL;
    int tiempoMigracionCrawler;
    int tiempoNuevaConsulta;
    char ipPortLDAP[40];            /*ip servidor LDAP*/
    char claveLDAP[20];             /*Clave servidor LDAP*/
    int log;
} configuracion;

int leerArchivoConfiguracion (configuracion *config);
int asignarDatos (configuracion *config, char *key, char *value);

#endif	/* _CONFIG_H */

