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
OBJS = frontend.o irc.o http.o config.o

frontend: $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o frontend $(OBJS)

clean:
	rm -f frontend $(OBJS)
