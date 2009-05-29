/* 
 * File:   ldap.c
 * Author: lucio
 *
 * Created on 9 de mayo de 2009, 4:52
 */

#include "mldap.h"

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
	if(ldap->session->started!=1)    	
	{
		perror("ldap startSession");
		return -1;
	}

	return 0;
}
/*
Descripcion: Inserta una entrada ldap URL segun entrada y mode
Ultima modificacion: Moya Farina, Lucio
Recibe: entrada con los datos modificados, tipo de archivo, cantidad de palabras
Devuelve: ok? 0: -1.
*/
int ldapAltaURL(ldapObj *ldap, crawler_URL* entrada, int mode, unsigned int cantidadPalabras)
{
    char uuid[MAX_UUID];
    char dn[MAX_UUID+25];
    int i;/*contardor para recorrer palabras*/

    /*Generar el identificador unico para el URL*/
    /*GenerarUUID(uuid);*/
    
    /*Crear entrada*/
    PLDAP_ENTRY entry = ldap->entryOp->createEntry();
    strcpy(dn,"utnurlID=");
    strcat(dn, uuid);
    strcat(dn,",ou=so,dc=utn,dc=edu");

    /*Se indica el tipo de entrada*/
    entry->dn = dn;

    ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("objectclass", 2, "top", "utnUrl"));
    ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnUrlID", 1, uuid));
    ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("labeledURL", 1, entrada->URL));
  /*  for (i=0; i<cantidadPalabras ;i++) REVISAR, NO ESTA BIEN EL TEMA DEL VOID *
        ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnurlKeywords", 1, entrada->palabras[i])); */

    /*Si es un Archivo guarda los campos espeficos del mismo*/
    if (mode == IRC_CRAWLER_ALTA_ARCHIVOS)
    {
        ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnUrlType", 1, entrada->tipo));
        ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnUrlFormat", 1, entrada->formato));
        ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnUrlSize", 1, entrada->length));
    }
    /*Si es un HTML guarda los campos espeficos del mismo*/
    else if (mode == IRC_CRAWLER_ALTA_HTML)
    {        
        ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnurlTitle", 1, entrada->titulo));
        ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnurlDescription", 1, entrada->descripcion));
        ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnurlContent", 1, entrada->htmlCode));
    }
    else
    {
        printf("ldapAltaURL: opcion inexistente");
        return -1;
    }
    
    /* Inserta la entry en el directorio */
    ldap->sessionOp->addEntry(ldap->session, entry);
    
    return 0;
}
/*
Descripcion: Modifica una entrada ldap URL segun entrada y mode
Ultima modificacion: Moya Farina, Lucio
Recibe: entrada con los datos modificados, tipo de archivo, cantidad de palabras
Devuelve: ok? 0: -1.
*/
int ldapModificarURL(ldapObj *ldap, crawler_URL* entrada, int mode, unsigned int cantidadPalabras)
{
    int i, control;
    PLDAP_ENTRY entry = ldap->entryOp->createEntry();
    char dn[DN_LEN];
    
    memset(dn, '\0', sizeof(DN_LEN));
    control = ldapObtenerDN(ldap, entrada->URL, mode, dn);
    if(!control)
    {
      printf("ldapModificarURL: Imposible modificar, no se encontro la entrada %s\n", entrada->URL);
      return -1;
    }
    if(control==-1)    
      return -1;
    

    entry->dn = dn;
       
    for (i=0; i<cantidadPalabras ;i++)
        ldap->entryOp->editAttribute(entry, ldap->attribOp->createAttribute("utnurlKeywords", 1, entrada->palabras[i]));


    /*Si es un Archivo guarda los campos espeficos del mismo*/
    if (mode == IRC_CRAWLER_MODIFICACION_ARCHIVOS)
    {
        ldap->entryOp->editAttribute(entry, ldap->attribOp->createAttribute("utnUrlType", 1, entrada->tipo));
        ldap->entryOp->editAttribute(entry, ldap->attribOp->createAttribute("utnUrlFormat", 1, entrada->formato));
        ldap->entryOp->editAttribute(entry, ldap->attribOp->createAttribute("utnUrlSize", 1, entrada->length));
    }
    /*Si es un HTML edita los campos espeficos del mismo*/
    else if (mode == IRC_CRAWLER_MODIFICACION_HTML)
    {
        ldap->entryOp->editAttribute(entry, ldap->attribOp->createAttribute("utnurlTitle", 1, entrada->titulo));
        ldap->entryOp->editAttribute(entry, ldap->attribOp->createAttribute("utnurlDescription", 1, entrada->descripcion));
        ldap->entryOp->editAttribute(entry, ldap->attribOp->createAttribute("utnurlContent", 1, entrada->htmlCode));
    }
    else
    {
        printf("ldapAltaURL: opcion inexistente\n");
        return -1;
    }

   ldap->sessionOp->editEntry(ldap->session, entry);

    return 0;
}
/*
Descripcion: Genera una tabla de hosts segun lo almacenado en el dir de expiracion de ldap
Ultima modificacion: Moya Farina, Lucio
Recibe: lista de hosts actuales, numero maximo de hosts
Devuelve: ok? 0: -1. lista de hosts actualizada, y numero maximo de hosts
*/
int ldapObtenerHosts(ldapObj *ldap, webServerHosts **hosts, int *maxHosts)
{
	/* hago una consulta en una determinada rama aplicando la siguiente condicion */
	PLDAP_RESULT_SET resultSet 	= ldap->sessionOp->searchEntry(ldap->session, "ou=hosts,dc=utn,dc=edu", "utnHostID=*");
	PLDAP_ITERATOR iterator = NULL;
	PLDAP_RECORD_OP	recordOp = newLDAPRecordOperations();
    int i, allocLen = sizeof(webServerHosts);

    *maxHosts = resultSet->count;
    if(*hosts = malloc(allocLen))
    {
      perror("malloc");
      return -1;
    }
	/* itero sobre los registros obtenidos a traves de un iterador que conoce la implementacion del recorset */
	for(i = 0, iterator = resultSet->iterator; iterator->hasNext(resultSet); i++)
    {
		PLDAP_RECORD record = iterator->next(resultSet);
        if(*hosts = realloc(*hosts, allocLen*(i+1)))
        {
          perror("remalloc");
          return -1;
        }

		/* Itero sobre los campos de cada uno de los record */
		while(recordOp->hasNextField(record))
        {
			PLDAP_FIELD field = recordOp->nextField(record);
			INT	index = 0;
            char *ip, *puerto;

            if(!(strcmp(field->name, "utnHostID")))
            {
              ip = strtok(field->values[index], ":");
              puerto = strtok(NULL, ":");
              (*hosts)[i].hostIP = inet_addr(ip);
              (*hosts)[i].hostPort = htons(atoi(puerto));
            }
            if(!(strcmp(field->name, "utnHostModTimestamp")))
              (*hosts)[i].uts = atoi(field->values[index]);

			/* se libera la memoria utilizada por el field si este ya no es necesario. */
			freeLDAPField(field);
		}
		/* libero los recursos consumidos por el record */
		freeLDAPRecord(record);
	}
	/* libero los recursos */
	freeLDAPIterator(iterator);
	freeLDAPRecordOperations(recordOp);

    return 0;
}
/*
Descripcion: Actualiza el dir de expiracion de ldap, con el tiempo enviado
Ultima modificacion: Moya Farina, Lucio
Recibe: nuevo uts, modo
Devuelve: ok? 0: -1. Indica para cada modo a que se debio el fallo
*/
int ldapActualizarHost(ldapObj *ldap, const char *ipPuerto, time_t nuevoUts, int mode)
{    
    PLDAP_ENTRY entry = ldap->entryOp->createEntry();
    char dn[DN_LEN];
    char ts[UTS_LEN];
    int existe;
    time_t ahora;

    memset(dn, '\0', sizeof(DN_LEN));
    memset(ts, '\0', sizeof(UTS_LEN));

    existe = ldapComprobarExistencia(ldap, ipPuerto, IRC_CRAWLER_HOST);
    
    if(existe && mode==ALTA)
    {
      printf("ldapActualizarHost: No se pudo agregar Host. Ya existe un host con esa entrada\n");
      return -1;
    }
    else if(!existe && mode==MODIFICACION)
    {
      printf("ldapActualizarHost: No se pudo modificar Host. No existe un host con esa entrada\n");
      return -1;
    }

    strcpy(dn, "utnHostID=");
    strcat(dn, ipPuerto);
    strcat(dn, ",ou=hosts,dc=utn,dc=edu");

    ahora = nuevoUts;
    strftime ( ts , sizeof ( ts ) , "%Y%m%d%H%M%SZ" , localtime ( &ahora ) ) ;
    ts[31] = '\0' ;

    entry->dn = dn;

    printf("%s\n", entry->dn);
    printf("%s\n", ts);

    if(mode == MODIFICACION)
    {
      ldap->entryOp->editAttribute(entry, ldap->attribOp->createAttribute("utnHostModTimestamp", 1, ts) );
      
      ldap->sessionOp->editEntry(ldap->session, entry);
    }
    else if(mode == ALTA)
    {
      ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("objectclass", 2, "top", "utnHost"));
      ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnHostID", 1, ipPuerto) );
      ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnHostModTimestamp", 1, ts) );

      ldap->sessionOp->addEntry(ldap->session, entry);
    }
    
    return 0;
}
/*
Descripcion: Obtiene la Direccion (DN) de una entrada LDAP segun una clave.
Ultima modificacion: Moya Farina, Lucio
Recibe: valor clave, tipo de archivo, string direccion (DN)
Devuelve: ok? 0: -1. DN completo, 0 si no existe, -1 errores
*/
int ldapObtenerDN(ldapObj *ldap,char *key, int mode, char *dn)
{  
  char filtro[MAX_PATH], directorio[MAX_PATH];
  PLDAP_RESULT_SET resultSet = NULL;
  
  if (mode == IRC_CRAWLER_MODIFICACION_HTML || mode == IRC_CRAWLER_MODIFICACION_ARCHIVOS)
  {
    strcpy(filtro,"(labeledURL=");
    strcpy(directorio, "ou=so,dc=utn,dc=edu");
  }
  else if (mode == IRC_CRAWLER_HOST)
  {
    strcpy(filtro,"(utnHostID=");
    strcpy(directorio, "ou=hosts,dc=utn,dc=edu");
  }
  else
  {
    printf("ldapObtenerDN: Error, modo inexistente");
    return -1;
  }

  strcat(filtro, key);
  strcat(filtro, ")");
 
  if ((resultSet = ldap->sessionOp->searchEntry(ldap->session, directorio, filtro) )==NULL )
    return 0;

  PLDAP_ITERATOR iterator = resultSet->iterator;
  PLDAP_RECORD record = iterator->next(resultSet);

  strcpy(dn,  record->dn);
  
  freeLDAPRecord(record);
  freeLDAPIterator(iterator);

  return 1;
}
/*
Descripcion: Comprueba la existencia de una entrada en la base LDAP.
Ultima modificacion: Moya Farina, Lucio
Recibe: valor clave, tipo de archivo
Devuelve: ok? 0: -1. 1 si existe, 0 si no existe, -1 errores
*/
int ldapComprobarExistencia(ldapObj *ldap, const char *clave, int mode)
{
    PLDAP_RESULT_SET resultSet = NULL;
    char busqueda[MAX_PATH+15];


    if (mode == IRC_CRAWLER_ALTA_HTML ||
        mode == IRC_CRAWLER_ALTA_ARCHIVOS ||
        mode == IRC_CRAWLER_MODIFICACION_HTML ||
        mode == IRC_CRAWLER_MODIFICACION_ARCHIVOS)
    {
        strcpy(busqueda,"(labeledURL=");
        strcat(busqueda, clave);
        strcat(busqueda, ")");
        resultSet = ldap->sessionOp->searchEntry(ldap->session, "ou=so,dc=utn,dc=edu", busqueda);
    }

    else if(mode == IRC_CRAWLER_HOST)
    {
        strcpy(busqueda,"(utnHostID=");
        strcat(busqueda, clave);
        strcat(busqueda, ")");        
        resultSet = ldap->sessionOp->searchEntry(ldap->session, "ou=hosts,dc=utn,dc=edu", busqueda);        
    }
    else
    {
        printf("ldapComprobarEsixtencia: opcion inexistente");
        return -1;
    }

    return resultSet->count > 1 ?  1: 0;
}
