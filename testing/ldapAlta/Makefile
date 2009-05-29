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
OBJS = LdapWrapper.o config.o irc.o mldap.o ldapAlta.o

frontend: $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o lda $(OBJS)

clean:
	rm -f lda $(OBJS)
