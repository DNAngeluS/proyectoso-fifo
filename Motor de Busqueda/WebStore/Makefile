# Grupo Fifo
# ---------------------
# Integrantes:
#	-Ariel Pacay
#   -Ariel Dias
#	-Lucio Moya Farina
#	-Mariano Schienkman
#----------------------
# Ayudante designado:
#	-Carla Caldarola
#----------------------

#Libs
THREADLIBS=-lpthread
LIBDIR=#/usr/lib
LDAPLIB=-lldap -llber

#Flags
CC = exec gcc-4.1
CFLAGS = -g -ansi -D_XOPEN_SOURCE=600 #-W -Wall -Werror -pedantic-errors -O2 -Wformat=2 -Wno-unused -DPOSIX -DPOSIX_SOURCE

#Dependencias
OBJS = config.o irc.o mldap.o LdapWrapper.o
OBJS_WS = webstore.o $(OBJS)
OBJS_CR = crawler-create.o $(OBJS)
LIBS=-L $(THREADLIBS) $(LIBDIR) $(LDAPLIB)

webstore: $(OBJS_WS)
	$(CC) $(CFLAGS) $(LIBS) -o webstore  $(OBJS_WS)

crawler-create: $(OBJS_CR)
	$(CC) $(CFLAGS) $(LIBS) -o crawler-create $(OBJS_CR)

clean:
	rm -f webstore crawler-create $(OBJS_WS) $(OBJS_CR)