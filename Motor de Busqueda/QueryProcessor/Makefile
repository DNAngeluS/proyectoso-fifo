# Grupo Fifo
# ---------------------
# Integrantes:
#	-Ariel Dias
#	-Ariel Pacay
#	-Lucio Moya Farina
#	-Mariano Schienkman
#----------------------
# Ayudante designado:
#	-Carla Cadarola
#----------------------

EXTRACFLAGS=
EXTRALDFLAGS=-lsocket -lnsl
# Estas dos lineas comentadas es por si usaban las librerias de netscape
#LDAPLIB=ldap50 -lplc4 -lplds4 -lnspr4
#LDAPLIB=-lldap50 -lplc4 -lplds4

# Estas son las librerias de openLdap
LDAPLIB=-lldap -llber

# en mi caso las librerias estan en /opt/csw/
# Si se bajan las librerias de http://www.blastwave.org (openldap_devel)
# van a parar ahi
INCDIR=/opt/csw/include
LIBDIR=/opt/csw/lib
LIBS=-R $(LIBDIR) -L $(LIBDIR) $(LDAPLIB) $(EXTRALDFLAGS)

#Flags
CC = exec /opt/csw/gcc4/bin/gcc
CFLAGS =-ansi -g# -Wall

#Dependencias
OBJS = LdapWrapper.o config.o http.o irc.o mldap.o qp.o

qp: $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o qp $(OBJS)

clean:
	rm -f qp $(OBJS)


