/* 
 * File:   ldap.h
 * Author: lucio
 *
 * Created on 9 de mayo de 2009, 4:53
 */

#ifndef _MLDAP_H
#define	_MLDAP_H

#include "LdapWrapper.h"

typedef struct {
    PLDAP_SESSION session;
    PLDAP_CONTEXT context;
    PLDAP_CONTEXT_OP ctxOp;
    PLDAP_SESSION_OP sessionOp;
    PLDAP_ENTRY_OP entryOp;
    PLDAP_ATTRIBUTE_OP attribOp;
} ldapObj;

int establecerConexionLDAP(ldapObj *ldap);
PLDAP_RESULT_SET consultarLDAP(ldapObj ldap, char *palabras, int searchType);
VOID *armarPayload(PLDAP_RESULT_SET resultSet, int searchType);


#endif	/* _MLDAP_H */
