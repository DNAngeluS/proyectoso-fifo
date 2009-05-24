/* 
 * File:   ldapAlta.c
 * Author: lucio
 *
 * Created on 22 de mayo de 2009, 19:18
 */

#include "mldap.h"

#define MAX_PATH 256

typedef struct {
    char URL            [MAX_PATH];
    char UUID           [MAX_UUID];
    char palabras       [MAX_PATH];
    char titulo         [MAX_PATH];
    char descripcion    [MAX_PATH];
} so_URL_HTML;

typedef struct {
    char URL            [MAX_PATH];
    char nombre         [MAX_PATH];
    char palabras       [MAX_PATH];
    char tipo           [2];
    char formato        [4];
    char length         [20];
} so_URL_Archivos;

/*
 * 
 */
VOID ldapAltaURL(ldapObj ldap, void* entrada, int mode) {
	/* creo una nueva entry.
	le agrego los parametros correspondientes */
  char UUID[MAX_UUID];
  char dn[MAX_PATH];
  GenerarUUID(&UUID);
  dn = strcat(UUID, ",ou=so,dc=utn,dc=edu")
  
  PLDAP_ENTRY entry = ldap->entryOp->createEntry();
  entry->dn = dn;
  entryOp->addAttribute(entry, ldap->attribOp->createAttribute("objectclass", 2, "top", "utnUrl"));
  entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnUrlID", 1, UUID));
  entryOp->addAttribute(entry, ldap->attribOp->createAttribute("labeledURL", 2, "labeledURL", "www.unaUrl.com"));
  entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnurlTitle", 1, "Un titulo para la url"));
  entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnurlDescription", 1, "Esta es la descripcion de la url esta"));
  entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnurlContent", 1, "<HTML> <HEAD> <TITLE>El contenido</title> </HEAD> <B>Esto es todo</B> </HTML>"));
  entryOp->addAttribute(entry, ldap->attribOp->createAttribute("utnurlKeywords", 3, "EstesUnKiwor", "Estesotro", "AcaHayUnoMAs"));
  
  /* inserto la entry en el directorio */
  ldap->sessionOp->addEntry(ldap->session, entry);
}
/**
 * Esta funcion muestra como insertar una nueva entry
 * con nuevos atributos y valores
 *
 */
VOID insertEntry(PLDAP_SESSION session, PLDAP_SESSION_OP sessionOp, PLDAP_ENTRY_OP entryOp, PLDAP_ATTRIBUTE_OP attribOp) {

	/* creo una nueva entry.
	le agrego los parametros correspondientes */
	PLDAP_ENTRY entry = entryOp->createEntry();

	entry->dn = "utnurlID=000011,ou=so,dc=utn,dc=edu";

	entryOp->addAttribute(entry, attribOp->createAttribute("objectclass", 2, "top", "utnUrl"));

	entryOp->addAttribute(entry, attribOp->createAttribute("utnUrlID", 1, "000011"));

	entryOp->addAttribute(entry, attribOp->createAttribute("labeledURL", 2, "labeledURL", "www.unaUrl.com"));

	entryOp->addAttribute(entry, attribOp->createAttribute("utnurlTitle", 1, "Un titulo para la url"));

	entryOp->addAttribute(entry, attribOp->createAttribute("utnurlDescription", 1, "Esta es la descripcion de la url esta"));

	entryOp->addAttribute(entry, attribOp->createAttribute("utnurlContent", 1, "<HTML> <HEAD> <TITLE>El contenido</title> </HEAD> <B>Esto es todo</B> </HTML>"));

	entryOp->addAttribute(entry, attribOp->createAttribute("utnurlKeywords", 3, "EstesUnKiwor", "Estesotro", "AcaHayUnoMAs"));


	/* inserto la entry en el directorio */
	sessionOp->addEntry(session, entry);
}
int main() {


	PLDAP_SESSION session;
	PLDAP_CONTEXT context = newLDAPContext();

	/* creo los objetos que me permiten operar sobre
	     un contexto y sobre una session */
	PLDAP_CONTEXT_OP 	ctxOp 		= newLDAPContextOperations();
	PLDAP_SESSION_OP 	sessionOp 	= newLDAPSessionOperations();
	PLDAP_ENTRY_OP 		entryOp 	= newLDAPEntryOperations();
	PLDAP_ATTRIBUTE_OP 	attribOp 	= newLDAPAttributeOperations();


	/* inicia el context de ejecucion de ldap
	     en el host especificado */
	ctxOp->initialize(context, "ldap://192.168.1.4:1389");


	/* creo una session para comenzar a operar
	     sobre el direcotio ldap */
	session = ctxOp->newSession(context, "cn=Directory Manager", "soogle");


	/* inicio la session con el server ldap */
	sessionOp->startSession(session);



	/* TODO: operacion de datos aqui */
	insertEntry(session, sessionOp, entryOp, attribOp);
	    /* modifyEntry(session, sessionOp, entryOp, attribOp);*/
	     /*deleteEntry(session, sessionOp, entryOp, attribOp); */
	/*selectEntries(session, sessionOp, entryOp, attribOp);*/


	/* cierro la session con el servidor */
	sessionOp->endSession(session);


	/*  libero los objectos de operaciones */
	freeLDAPSession(session);
	freeLDAPContext(context);
	freeLDAPContextOperations(ctxOp);
	freeLDAPSessionOperations(sessionOp);

	return 0;

  return (EXIT_SUCCESS);
}

/*
Descripcion: Genera un descriptorID aleatorio
Ultima modificacion: Scheinkman, Mariano
Recibe: cadena vacia.
Devuelve: cadena con el nuevo descriptorID
*/
void GenerarUUID(char *cadenaOUT){

	int i;

	srand (time (NULL));
	for(i=0; i<DESCRIPTORID_LEN-1; i++)
	{
		cadenaOUT[i] = 'A'+ ( rand() % ('Z' - 'A') );
	}

	cadenaOUT[i] = '\0';

}

