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
    in_addr_t ip;		/*ip local*/
    in_port_t puerto;		/*puerto*/
    in_addr_t ipQM;		/*ip local*/
    in_port_t puertoQM;		/*puerto*/
    char ipPortLDAP[30];	/*ip servidor LDAP*/
    char claveLDAP[20];		/*Clave servidor LDAP*/
    int tipoRecurso;            /*Tipo de recurso que atendera*/
    int cantidadConexiones;     /*Cantidad de conexiones simultaneas*/
    int tiempoDemora;           /*Tiempo de demora en enviar rta al Front-end*/
} configuracion;

int leerArchivoConfiguracion (configuracion *config);
int asignarDatos (configuracion *config, char *key, char *value);

#endif	/* _CONFIG_H */

