/* 
 * File:   ldapAlta.c
 * Author: lucio
 *
 * Created on 22 de mayo de 2009, 19:18
 */

#include "ldapAlta.h"
#include "mldap.h"

void GenerarUUID(char *cadenaOUT);

int main() {
  ldapObj ldap;

  ldap.session;
  ldap.context = newLDAPContext();
  ldap.ctxOp = newLDAPContextOperations();
  ldap.sessionOp = newLDAPSessionOperations();
  ldap.entryOp = newLDAPEntryOperations();
  ldap.attribOp = newLDAPAttributeOperations();


  printf("Se va a establecer la conexion LDAP\n\n");
   /* inicia el context de ejecucion de ldap en el host especificado */
  ldap.ctxOp->initialize(ldap.context, "ldap://192.168.1.4:1389");
  /* creo una session para comenzar a operar sobre el direcotio ldap */
  ldap.session = ldap.ctxOp->newSession(ldap.context, "cn=Directory Manager", "soogle");
  /* inicio la session con el server ldap */
  ldap.sessionOp->startSession(ldap.session);
  printf("Conexion LDAP Establecida\n\n");
  /* TODO: operacion de datos aqui */

  printf("Crear Registro\n\n");
  webstore_URL datos;
  strcpy(datos.palabras,"pal1");  
  strcpy(datos.URL,"www.unaUrl.com");  
  strcpy(datos.titulo,"Un titulo para la url");  
  strcpy(datos.descripcion,"Esta es la descripcion de la url esta");  
  strcpy(datos.htmlCode,"<HTML> <HEAD> <TITLE>El contenido</title> </HEAD> <B>Esto es todo</B> </HTML>");
  
  /*PUREBAS*/
  /*ldapAltaURL(&ldap, &datos, IRC_RESPONSE_HTML);*/
  if(ldapComprobarExistencia(ldap, "129.0.1.82:90", IRC_RESPONSE_HOST)!=-1)
    printf("Se encontro\n");
  else
    printf("No esta en Dir SO");

  
  /* cierro la session con el servidor */
  ldap.sessionOp->endSession(ldap.session);
  /*  libero los objectos de operaciones */
  freeLDAPSession(ldap.session);
  freeLDAPContext(ldap.context);
  freeLDAPContextOperations(ldap.ctxOp);
  freeLDAPSessionOperations(ldap.sessionOp);
  return 0;
}

/*
Descripcion: Genera un descriptorID aleatorio
Ultima modificacion: Scheinkman, Mariano
Recibe: cadena vacia.
Devuelve: cadena con el nuevo descriptorID
*/
void GenerarUUID(char *cadenaOUT)
{
  /*HACER*/
}