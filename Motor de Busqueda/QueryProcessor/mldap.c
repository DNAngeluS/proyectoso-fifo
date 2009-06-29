/* 
 * File:   ldap.c
 * Author: lucio
 *
 * Created on 9 de mayo de 2009, 4:52
 */

#include "mldap.h"

void generarArrayHTML    (so_URL_HTML * resultados, PLDAP_FIELD field, int pos);
void generarArrayOTROS   (so_URL_Archivos * resultados, PLDAP_FIELD field, int pos);
void generarArrayCACHE   (hostsCodigo *resultados, PLDAP_FIELD field, int pos);

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
	if(ldap->context->errorCode != LDAP_SUCCESS)
	{
		perror("ldap initialize");
		return -1;
	}

	ldap->session = ldap->ctxOp->newSession(ldap->context, "cn=Directory Manager", config.claveLDAP);
	if(ldap->session == NULL)
	{
		perror("ldap newSession");
		return -1;
	}

	ldap->sessionOp->startSession(ldap->session);
	if (ldap->session->started != 1)
	{
		perror("ldap startSession");
		return -1;
	}

	return 0;
}

PLDAP_RESULT_SET consultarLDAP(ldapObj *ldap, char querystring[QUERYSTRING_SIZE])
{
    return ldap->sessionOp->searchEntry(ldap->session, "ou=so,dc=utn,dc=edu", querystring);
}


int armarPayload(PLDAP_RESULT_SET resultSet, void **resultados, int mode, unsigned int *cantBloques)
{
    /* Hace una consulta en una determinada rama aplicando la siguiente condicion*/
    PLDAP_ITERATOR iterator = NULL;
    PLDAP_RECORD_OP recordOp = newLDAPRecordOperations();
    int allocLen, i;

    *cantBloques = 0;

    /*Establece el tamaño de la estructura a allocar*/
    if (mode == IRC_REQUEST_HTML)
        allocLen = sizeof(so_URL_HTML);
    else if (mode == IRC_REQUEST_ARCHIVOS)
        allocLen = sizeof(so_URL_Archivos);
    else if (mode == IRC_REQUEST_CACHE)
        allocLen = sizeof(hostsCodigo);
    else
    {
        perror("Armar payload: inconcistencia de payload descriptor");
        return -1;
    }

    /*Aloca memoria para la estructura*/
    if ((*resultados = malloc(allocLen)) == NULL)
    {
        perror("malloc");
        return -1;
    }
    memset(*resultados, '\0', allocLen);

    /* Itera sobre los registros obtenidos a traves de un iterador que conoce
     * la implementacion del recorset*/
    for(i = 0, iterator = resultSet->iterator; iterator->hasNext(resultSet); i++)
    {
        PLDAP_RECORD record = iterator->next(resultSet);

        /*Realoca memoria para la estructura*/
        if ((*resultados = realloc(*resultados, allocLen*(i+1))) == NULL)
        {
            perror("realloc");
            return -1;
        }
        memset(*resultados+(allocLen*i),'\0', allocLen);

        /*Itero sobre los campos de cada uno de los records*/
        while(recordOp->hasNextField(record))
        {
            PLDAP_FIELD field = recordOp->nextField(record);

            /*Asigna los valores a la estructura*/
            if (mode == IRC_REQUEST_HTML)
                generarArrayHTML((so_URL_HTML *) *resultados, field, i);
            else if (mode == IRC_REQUEST_ARCHIVOS)
                generarArrayOTROS((so_URL_Archivos *) *resultados, field, i);
            else if (mode == IRC_REQUEST_CACHE)
                generarArrayCACHE((hostsCodigo *) *resultados, field, i);

            /*Se libera la memoria utilizada por el field*/
            freeLDAPField(field);
        }

        *cantBloques = *cantBloques + 1;

        /* Libero los recursos consumidos por el record*/
        freeLDAPRecord(record);
    }

    /* Libero los recursos*/
    freeLDAPIterator(iterator);
    freeLDAPRecordOperations(recordOp);

    return 0;
}

void ldapFreeResultSet(PLDAP_RESULT_SET resultSet)
{
    PLDAP_ITERATOR iterator = NULL;
    PLDAP_RECORD_OP recordOp = newLDAPRecordOperations();
    int i;

    for(i = 0, iterator = resultSet->iterator; iterator->hasNext(resultSet); i++)
    {
        PLDAP_RECORD record = iterator->next(resultSet);

        while(recordOp->hasNextField(record))
        {
            PLDAP_FIELD field = recordOp->nextField(record);
            freeLDAPField(field);
        }

        freeLDAPRecord(record);
    }
    freeLDAPIterator(iterator);
    freeLDAPRecordOperations(recordOp);
}

/*Devuelve 1 si agrego una entrada. 0 si no agrego ninguna*/
void generarArrayCACHE(hostsCodigo *resultados, PLDAP_FIELD field, int pos)
{
    int	index = 0;

    if (!(strcmp(field->name,"utnurlID")))
        strcpy(resultados[pos].UUID, field->values[index]);
    else if (!(strcmp(field->name,"utnurlContent")))
        strcpy(resultados[pos].html, field->values[index]);
    else
        return;
}

/*Devuelve 1 si agrego una entrada. 0 si no agrego ninguna*/
void generarArrayHTML(so_URL_HTML *resultados, PLDAP_FIELD field, int pos)
{
    int	index = 0;

    if (!(strcmp(field->name,"utnurlID")))
        strcpy(resultados[pos].UUID, field->values[index]);
    else if (!(strcmp(field->name,"utnurlTitle")))
        strcpy(resultados[pos].titulo, field->values[index]);
    else if (!(strcmp(field->name,"utnurlDescription")))
        strcpy(resultados[pos].descripcion, field->values[index]);
    else if (!(strcmp(field->name,"labeledURL")))
        strcpy(resultados[pos].URL, field->values[index]);
    else if (!(strcmp(field->name,"utnurlKeywords")))
    {
	char buf[MAX_PATH];

        memset(buf, '\0', MAX_PATH);
        strcpy (buf, field->values[index]);
        for(index = 1; index < field->valuesSize; index++)
        {
            strcat(buf, ", ");
            strcat(buf, field->values[index]);            
        }
	strcpy(resultados[pos].palabras, buf);
    }
    else return;
}

/*Devuelve 1 si agrego una entrada. 0 si no agrego ninguna*/
void generarArrayOTROS(so_URL_Archivos *resultados, PLDAP_FIELD field, int pos)
{
    int	index = 0;

    if (!(strcmp(field->name,"utnurlNombre")))
        strcpy(resultados[pos].nombre, field->values[index]);
    else if (!(strcmp(field->name,"utnurlFormat")))
        strcpy(resultados[pos].formato, field->values[index]);
    else if (!(strcmp(field->name,"utnurlType")))
        strcpy(resultados[pos].tipo, field->values[index]);
    else if (!(strcmp(field->name,"utnurlLen")))
        strcpy(resultados[pos].length,field->values[index]);
    else if (!(strcmp(field->name,"labeledURL")))
        strcpy(resultados[pos].URL, field->values[index]);
    else if (!(strcmp(field->name,"utnurlKeywords")))
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
    else return;
}
