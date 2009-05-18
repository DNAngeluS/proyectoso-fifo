/* 
 * File:   config.c
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 20:08
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>

int leerArchivoConfiguracion (configuracion *config)
{
    in_addr_t ip = inet_addr("127.0.0.1");			/*ip local*/
    in_port_t puerto = htons(3490);		            /*puerto*/
    char ipPortLDAP[30] = "ldap://127.0.0.1:1389";	/*ip servidor LDAP*/
    char claveLDAP[20] = "soogle";		            /*Clave servidor LDAP*/
    
    return 0;
}
int asignarDatos (configuracion *config, char *key, char *value)
{
    /*HACER*/
}
