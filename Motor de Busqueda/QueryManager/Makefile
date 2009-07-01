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

#Libs
SOCKLIBS=-lsocket -lnsl
LIBDIR=
#Flags
CC = exec /opt/csw/gcc4/bin/gcc
CFLAGS =-ansi -g# -Wall
LIBS=-R -L $(SOCKLIBS)

#Dependencias
OBJS = qmanager.o irc.o querylist.o rankinglist.o config.o

qmanager: $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o qmanager $(OBJS)

clean:
	rm -f qmanager $(OBJS)
