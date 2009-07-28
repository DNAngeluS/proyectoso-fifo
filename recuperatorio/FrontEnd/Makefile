# Grupo Fifo
#
# Integrantes:
#	Lucio Moya Farina
#	Mariano Scheinkman
#
# Makefile - Front-end

#Libs
SOCKLIBS=-lsocket -lnsl
LIBDIR=
#Flags
CC = exec /opt/csw/gcc4/bin/gcc
CFLAGS =-ansi -g# -Wall
LIBS=-R -L $(SOCKLIBS)

#Dependencias
OBJS = frontend.o irc.o http.o config.o log.o

frontend: $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o frontend $(OBJS)

clean:
	rm -f frontend $(OBJS)
