/* 
 * File:   ldap.c
 * Author: lucio
 *
 * Created on 9 de mayo de 2009, 4:52
 */

#include "mldap.h"

void generarArrayHTML(so_URL_HTML * resultados, PLDAP_FIELD field, int pos);
void generarArrayOTROS(so_URL_Archivos * resultados, PLDAP_FIELD field, int pos);

int establecerConexionLDAP(ldapObj *ldap, configuracion config)
{
    /*Variables del LDAP*/
    ldap->session;
    ldap->context = newLDAPContext();

    ldap->ctxOp = newLDAPContextOperations();
    ldap->sessionOp = newLDAPSessionOperations();
    ldap->entryOp = newLDAPEntryOperations();
    ldap->attribOp = newLDAPAttributeOperations();

    /*Se establece la conexion con LDAP*/
    ldap->ctxOp->initialize(ldap->context, config.ipPortLDAP);
    if(ldap->context->errorCode!=LDAP_SUCCESS)
	return 1;
    ldap->session = ldap->ctxOp->newSession(ldap->context, "cn=Directory Manager", config.claveLDAP);
    if(ldap->session == NULL)
	return 1;
    ldap->sessionOp->startSession(ldap->session);	
    if(ldap->session->started!=1)    	
	return 1;
    
    return 0;
}

PLDAP_RESULT_SET consultarLDAP(ldapObj ldap, char buf[QUERYSTRING_SIZE], int searchType)
{
    PLDAP_RESULT_SET resultSet=NULL;

    if (searchType == WEB)
        resultSet = ldap.sessionOp->searchEntry(ldap.session, "ou=so,dc=utn,dc=edu", buf);
    if ((searchType == IMG) || (searchType == OTROS))
        resultSet = ldap.sessionOp->searchEntry(ldap.session, "ou=so,dc=utn,dc=edu", buf);

    return resultSet;
}


VOID* armarPayload(PLDAP_RESULT_SET resultSet, int searchType)
{
    /* Hace una consulta en una determinada rama aplicando la siguiente condicion*/    
    PLDAP_ITERATOR iterator = NULL;
    PLDAP_RECORD_OP recordOp = newLDAPRecordOperations();
    VOID *resultados = NULL;
    int i;
    /*Aloca memoria para la estructura*/
    if (searchType == WEB) resultados = (so_URL_HTML *) 
	malloc (sizeof(so_URL_HTML));
    if ((searchType == IMG) || (searchType == OTROS))
        resultados = (so_URL_Archivos *) malloc (sizeof(so_URL_Archivos));

    /* Itera sobre los registros obtenidos a traves de un iterador que conoce
     * la implementacion del recorset*/
    for(i = 0, iterator = resultSet->iterator; iterator->hasNext(resultSet); i++)
    {
        PLDAP_RECORD record = iterator->next(resultSet);
        /*Realoca memoria para la estructura*/
        if (searchType == WEB) 
	    resultados = (so_URL_HTML *) realloc(resultados, sizeof(so_URL_HTML));
        if ((searchType == IMG) || (searchType == OTROS))
            resultados = (so_URL_Archivos *) realloc(resultados, sizeof(so_URL_Archivos));
	        
        /* Itero sobre los campos de cada uno de los record*/
        while(recordOp->hasNextField(record))
        {
            PLDAP_FIELD field = recordOp->nextField(record);                        
            /*Por cada campo del record llama a la funcion correspondiente y asigna los valores
	     *a la estructura*/
            if (searchType == WEB)
                generarArrayHTML((so_URL_HTML *) resultados, field, i);
            if ((searchType == IMG) || (searchType == OTROS))
                generarArrayOTROS((so_URL_Archivos *) resultados, field, i);
        /* Se libera la memoria utilizada por el field si este ya no es
         * necesario.*/
         freeLDAPField(field);
        }

        /* Libero los recursos consumidos por el record*/
        freeLDAPRecord(record);
    }

    /* Libero los recursos*/
    freeLDAPIterator(iterator);
    freeLDAPRecordOperations(recordOp);

    return resultados;
}

void generarArrayHTML(so_URL_HTML *resultados, PLDAP_FIELD field, int pos)
{
    INT	index = 0;    
    if(!(strcmp(field->name,"utnurlID")))strcpy(resultados[pos].UUID, field->values[index]);
    if(!(strcmp(field->name,"utnurlTitle")))strcpy(resultados[pos].titulo, field->values[index]);
    if(!(strcmp(field->name,"utnurlDescription")))strcpy(resultados[pos].descripcion, field->values[index]);
    if(!(strcmp(field->name,"labeledURL")))strcpy(resultados[pos].URL, field->values[index]);
    if(!(strcmp(field->name,"utnurlKeywords")))
    {
	char buf[BUF_SIZE] = "";
        strcat (buf, field->values[index]);        
        for(index = 1; index < field->valuesSize; index++)
        {
            strcat(buf, ", ");
            strcat(buf, field->values[index]);            
        }
	strcpy(resultados[pos].palabras, buf);
    }

}
void generarArrayOTROS(so_URL_Archivos *resultados, PLDAP_FIELD field, int pos)
{
    INT	index = 0;

    if(!(strcmp(field->name,"utnurlNombre")))strcpy(resultados[pos].nombre, field->values[index]);
    if(!(strcmp(field->name,"utnurlFormat")))strcpy(resultados[pos].formato, field->values[index]);
    if(!(strcmp(field->name,"utnurlType")))strcpy(resultados[pos].tipo, field->values[index]);
    if(!(strcmp(field->name,"utnurlLen")))strcpy(resultados[pos].length,field->values[index]);
    if(!(strcmp(field->name,"labeledURL")))strcpy(resultados[pos].URL, field->values[index]);
    if(!(strcmp(field->name,"utnurlKeywords")))
    {
	char buf[BUF_SIZE] = "";
        strcat (buf, field->values[index]);
        for(index = 1; index < field->valuesSize; index++)
        {
            strcat(buf, ", ");
            strcat(buf, field->values[index]);
        }
	strcpy(resultados[pos].palabras, buf);
    }
}
