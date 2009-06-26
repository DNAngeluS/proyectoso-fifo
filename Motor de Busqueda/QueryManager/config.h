/* 
 * File:   config.h
 * Author: marianoyfer
 *
 * Created on 19 de junio de 2009, 20:28
 */

#ifndef _CONFIG_H
#define	_CONFIG_H

typedef unsigned int in_addr_t;
typedef unsigned short in_port_t;

typedef struct {
    in_addr_t ipL;                        /*ip local*/
    in_port_t puertoL;                /*puerto local*/
    in_addr_t ipFrontEnd;          /*ip local*/
    in_port_t puertoFrontEnd;  /*puerto local*/
} configuracion;

int leerArchivoConfiguracion (configuracion *config);
int asignarDatos (configuracion *config, char *key, char *value);

#endif	/* _CONFIG_H */

