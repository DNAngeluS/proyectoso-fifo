#include "ld.h"
#include "LdapWrapper.h"
#include "irc.h"
#include "mldap.h"



int main(void){
	ldapObj ldap;
	INT control;
	
	PLDAP_RESULT_SET resultSet;
	so_URL_HTML *resultados;

	resultSet = NULL;
	resultados = NULL;

	if(establecerConexionLDAP(&ldap)!=0)
		printf("No se pudo establecer la conexion LDAP.\n");	
	else
		printf("Conexion establecida con LDAP\n");
	
	resultSet = consultarLDAP(ldap,"", WEB);

	resultados = (so_URL_HTML *)armarPayload(resultSet, WEB);

	if(resultados!=NULL)
		printf("Encontro: %s\n",resultados[0].titulo);
	else
		printf("Error en ArmarPayload");	

	return 0;
}
