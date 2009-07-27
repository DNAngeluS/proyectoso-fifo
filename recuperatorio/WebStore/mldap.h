#ifndef _MLDAP_H
#define	_MLDAP_H

#include "config.h"
#include "LdapWrapper.h"
#include "irc.h"

#define DN_LEN 100
#define UTS_LEN 40

enum hosts_t {ALTA, MODIFICACION};

typedef struct {
    PLDAP_SESSION session;
    PLDAP_CONTEXT context;
    PLDAP_CONTEXT_OP ctxOp;
    PLDAP_SESSION_OP sessionOp;
    PLDAP_ENTRY_OP entryOp;
    PLDAP_ATTRIBUTE_OP attribOp;
} ldapObj;

typedef struct {
    in_addr_t hostIP;
    in_port_t hostPort;
    time_t uts;
} webServerHosts;

int establecerConexionLDAP(ldapObj *ldap, configuracion config);
int ldapObtenerDN(ldapObj *ldap,char *key, int mode, char *dn);
void ldapFreeResultSet(PLDAP_RESULT_SET resultSet);

int ldapAltaURL(ldapObj *ldap, crawler_URL* entrada, int mode);
int ldapModificarURL(ldapObj *ldap, crawler_URL* entrada, int mode);
int ldapComprobarExistencia(ldapObj *ldap, const char *clave, int mode);

int ldapActualizarHost(ldapObj *ldap, const char *ipPuerto, time_t nuevoUts, int mode);
int ldapObtenerHosts(ldapObj *ldap, webServerHosts **hosts, int *maxHosts);



#endif	/* _MLDAP_H */
