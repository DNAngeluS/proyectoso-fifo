#ifndef _MLDAP_H
#define	_MLDAP_H

#include "LdapWrapper.h"
#include "ldapAlta.h"

typedef struct {
    PLDAP_SESSION session;
    PLDAP_CONTEXT context;
    PLDAP_CONTEXT_OP ctxOp;
    PLDAP_SESSION_OP sessionOp;
    PLDAP_ENTRY_OP entryOp;
    PLDAP_ATTRIBUTE_OP attribOp;
} ldapObj;

int establecerConexionLDAP(ldapObj *ldap, configuracion config);
PLDAP_RESULT_SET consultarLDAP(ldapObj ldap, char *palabras, int mode);
VOID ldapAltaURL(ldapObj *ldap, webstore_URL* entrada, int mode);
int ldapComprobarExistencia(ldapObj ldap, char clave[MAX_PATH], int mode);



#endif	/* _MLDAP_H */
