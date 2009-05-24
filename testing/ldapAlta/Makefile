# Lo atamo' con alambre
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

INCDIR=/usr/include
#Libs
SOCKLIBS=
LIBDIR=/usr/lib
LDAPLIB=-lldap -llber
#Flags
CC = exec gcc
CFLAGS =-ansi -g
LIBS= -L $(LIBDIR) $(SOCKLIBS) $(LDAPLIB)
#Dependencias
OBJS = LdapWrapper.o irc.o mldap.o ldapAlta.o

frontend: $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o ldapAlta $(OBJS)

clean:
	rm -f wstore $(OBJS)
