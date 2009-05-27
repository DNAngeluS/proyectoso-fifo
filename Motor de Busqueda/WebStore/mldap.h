#ifndef _MLDAP_H
#define	_MLDAP_H

#include "config.h"
#include "LdapWrapper.h"
#include "irc.h"

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
int ldapAltaURL(ldapObj *ldap, crawler_URL* entrada, int mode, unsigned int sizePalabras);
int ldapComprobarExistencia(ldapObj ldap, char clave[MAX_PATH], int mode);



#endif	/* _MLDAP_H */
