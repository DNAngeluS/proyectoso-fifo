/* 
 * File:   config.h
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 20:08
 */

#ifndef _CONFIG_H
#define	_CONFIG_H

typedef unsigned int in_addr_t;
typedef unsigned short in_port_t;

typedef struct {
    in_addr_t ip;	/*ip local*/
    in_port_t puerto;	/*puerto*/    
} configuracion;

int leerArchivoConfiguracion (configuracion *config);
int asignarDatos (configuracion *config, char *key, char *value);

#endif	/* _CONFIG_H */

