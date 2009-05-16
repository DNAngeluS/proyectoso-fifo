/* 
 * File:   ldap.c
 * Author: lucio
 *
 * Created on 9 de mayo de 2009, 4:52
 */
#include <stdio.h>
#include <stdlib.h>
#include "mldap.h"
#include "irc.h"

int generarArrayHTML(so_URL_HTML * resultados, PLDAP_FIELD field, int pos);
int generarArrayOTROS(so_URL_Archivos * resultados, PLDAP_FIELD field, int pos);

ldapObj establecerConexionLDAP(configuracion c){
    ldapObj ldap;

    /*Variables del LDAP*/
    ldap.session;
    ldap.context = newLDAPContext();

    ldap.ctxOp = newLDAPContextOperations();
    ldap.sessionOp = newLDAPSessionOperations();
    ldap.entryOp = newLDAPEntryOperations();
    ldap.attribOp = newLDAPAttributeOperations();

    /*Se establece la conexion con LDAP*/
    ldap.ctxOp->initialize(ldap.context, "ldap://192.168.1.7:1389");
    ldap.session = ldap.ctxOp->newSession(ldap.context, "cn=Directory Manager", "soogle");
    ldap.sessionOp->startSession(ldap.session);

    return ldap;
}

PLDAP_RESULT_SET consultarLDAP(ldapObj ldap, char *palabras, int searchType) {

    PLDAP_RESULT_SET resultSet;

    if (searchType == WEB)
        resultSet = ldap.sessionOp->searchEntry(ldap.session, "ou=so,dc=utn,dc=edu", "utnurlKeywords=KW1");
    if ((searchType == IMG) || (searchType == OTROS))
        resultSet = ldap.sessionOp->searchEntry(ldap.session, "ou=so,dc=utn,dc=edu", "utnurlKeywords=KW1");

    return resultSet;
}


VOID* armarPayload(PLDAP_RESULT_SET resultSet, int searchType) {

    /* Hago una consulta en una determinada
     * rama aplicando la siguiente condicion*/    
    PLDAP_ITERATOR iterator = NULL;
    PLDAP_RECORD_OP recordOp = newLDAPRecordOperations();
    void *resultados;
    int i;
    /*Aloca memoria para la estructura*/
    if (searchType == WEB) resultados = (so_URL_HTML *) malloc (sizeof(so_URL_HTML));
    if ((searchType == IMG) || (searchType == OTROS))
        resultados = (so_URL_Archivos *) malloc (sizeof(so_URL_Archivos));

    /* Itero sobre los registros obtenidos
     * a traves de un iterador que conoce
     * la implementacion del recorset*/
    for(i = 0, iterator = resultSet->iterator; iterator->hasNext(resultSet); i++) {
        PLDAP_RECORD record = iterator->next(resultSet);
        /*Realoca memoria para la estructura*/
        if (searchType == WEB) realloc(resultados, sizeof(so_URL_HTML));
        if ((searchType == IMG) || (searchType == OTROS))
            realloc(resultados, sizeof(so_URL_Archivos));
        
        /* Itero sobre los campos de cada
         * uno de los record*/
        while(recordOp->hasNextField(record)) {
            PLDAP_FIELD field = recordOp->nextField(record);                        
            /*
            utnurlKeywords
            labeledURL
            utnurlTitle
            utnurlDescription
            utnurlID
            */
            if (searchType == WEB)
                generarArrayHTML((so_URL_HTML *) resultados, field, i);
            if ((searchType == IMG) || (searchType == OTROS))
                generarArrayOTROS((so_URL_Archivos *) resultados, field, i);
        /* Se libera la memoria utilizada
         * por el field si este ya no es
         * necesario.*/
         freeLDAPField(field);
        }

        /* Libero los recursos consumidos
         * por el record*/
        freeLDAPRecord(record);
    }

    /* Libero los recursos*/
    freeLDAPIterator(iterator);
    freeLDAPRecordOperations(recordOp);

    return resultados;
}

int generarArrayHTML(so_URL_HTML *resultados, PLDAP_FIELD field, int pos){
    INT	index = 0;
    
    if(field->name=="utnurlID")resultados[pos].UUID =(char *) field->values[index];
    if(field->name=="utnurlTitle")resultados[pos].titulo =(char *) field->values[index];
    if(field->name=="utnurlDescription")resultados[pos].descripcion =(char *) field->values[index];
    if(field->name=="labeledURL")resultados[pos].URL =(char *) field->values[index];
    if(field->name=="utnurlKeywords"){
        sprintf(resultados[pos].palabras, "%s", field->values[index]);
        for(index = 1; index < field->valuesSize; index++) {
            sprintf(resultados[pos].palabras, ", %s", field->values[index]);
        }
    }

}
int generarArrayOTROS(so_URL_Archivos *resultados, PLDAP_FIELD field, int pos){
    INT	index = 0;

    if(field->name=="utnurlID")resultados[pos].URL = field->values[index];
    if(field->name=="utnurlNombre")resultados[pos].nombre = field->values[index];
    if(field->name=="utnurlFormat")resultados[pos].formato = field->values[index];
    if(field->name=="utnurlType")resultados[pos].tipo = field->values[index];
    if(field->name=="utnurlLen")resultados[pos].length = field->values[index];
    if(field->name=="labeledURL")resultados[pos].URL = field->values[index];
    if(field->name=="utnurlKeywords"){
        sprintf(resultados[pos].palabras, "%s", field->values[index]);
        for(index = 1; index < field->valuesSize; index++) {
            sprintf(resultados[pos].palabras, ", %s", field->values[index]);
        }
    }

}
