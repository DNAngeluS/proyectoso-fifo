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
CFLAGS = -g# -ansi

#Dependencias
OBJS = wserv.o irc.o
LIBS=-L $(THREADLIBS)

wserv: $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o wserv  $(OBJS)


clean:
	rm -f wserv $(OBJS)
