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

#Flags
CC = exec gcc-4.1
CFLAGS = -g  -ansi

#Dependencias
OBJS_WS = webstore.o config.o
OBJS_CR = crawler-create.o irc.o
LIBS=-L $(THREADLIBS)

webstore: $(OBJS_WS)
	$(CC) $(CFLAGS) $(LIBS) -o webstore  $(OBJS_WS)

crawler-create: $(OBJS_CR)
	$(CC) $(CFLAGS) -o crawler-create $(OBJS_CR)

clean:
	rm -f webstore crawler-create $(OBJS_WS) $(OBJS_CR)