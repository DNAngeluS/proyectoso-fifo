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

PLDAP_RESULT_SET consultarLDAP(ldapObj ldap, char buf[QUERYSTRING_SIZE], int mode)
{
    PLDAP_RESULT_SET resultSet = NULL;

    if (mode == IRC_REQUEST_HTML)
        resultSet = ldap.sessionOp->searchEntry(ldap.session, "ou=so,dc=utn,dc=edu", buf);
    if (mode == IRC_REQUEST_ARCHIVOS)
        resultSet = ldap.sessionOp->searchEntry(ldap.session, "ou=so,dc=utn,dc=edu", buf);

    return resultSet;
}

VOID ldapAltaURL(ldapObj *ldap, webstore_URL* entrada, int mode)
{
  char UUID[MAX_UUID];
  char dn[MAX_UUID+25]="utnurlID=";
  int i;/*contardor para recorrer palabras*/

  strcpy(UUID, "000111");
  /*GenerarUUID(UUID);  */

  /*Crear entrada*/
  PLDAP_ENTRY entry = ldap->entryOp->createEntry();
  strcat(dn, UUID);
  strcat(dn,",ou=so,dc=utn,dc=edu");
  /*Se indica el tipo de entrada*/
  entry->dn = dn;
  ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("objectclass", 2, "top", "utnUrl"));
  /*Completar los campos*/
  ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("labeledURL", 2, "labeledURL", entrada->URL));
  /*Cambiar por un For que recorra todo el array de palabras */
    ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnurlKeywords", 1, entrada->palabras));
  /*Si es un Archivo guarda los campos espeficos del mismo*/
  if(mode == IRC_RESPONSE_ARCHIVOS)
  {
    ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnUrlType", 1, entrada->tipo));
    ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnUrlFormat", 1, entrada->formato));
    ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnUrlSize", 1, entrada->length));
  }
  /*Si es un HTML guarda los campos espeficos del mismo*/
  if(mode == IRC_RESPONSE_HTML)
  {
    ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnUrlID", 1, UUID));
    ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnurlTitle", 1, entrada->titulo));
    ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnurlDescription", 1, entrada->descripcion));
    ldap->entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnurlContent", 1, entrada->htmlCode));
  }
  /* Inserta la entry en el directorio */
  ldap->sessionOp->addEntry(ldap->session, entry);
}

int ldapComprobarExistencia(ldapObj ldap, char clave[MAX_PATH], int mode)
{
    PLDAP_RESULT_SET resultSet = NULL;
    char busqueda[MAX_PATH+15];
    if((mode == IRC_RESPONSE_HTML)||(mode == IRC_RESPONSE_ARCHIVOS))
      strcpy(busqueda,"(labeledURL=");
    if(mode == IRC_RESPONSE_HOST)
      strcpy(busqueda,"(utnHostID=");

    strcat(busqueda, clave);
    strcat(busqueda, ")");
    if((mode == IRC_RESPONSE_HTML)||(mode == IRC_RESPONSE_ARCHIVOS))
      resultSet = ldap.sessionOp->searchEntry(ldap.session, "ou=so,dc=utn,dc=edu", busqueda);
    if(mode == IRC_RESPONSE_HOST)
      resultSet = ldap.sessionOp->searchEntry(ldap.session, "ou=hosts,dc=utn,dc=edu", busqueda);

    if(resultSet!=NULL)
      return 0;
    else
      return -1;
}


