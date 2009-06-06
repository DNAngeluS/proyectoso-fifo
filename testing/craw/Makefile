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
OBJS = craw.o irc.o
LIBS=-L $(THREADLIBS)

craw: $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o craw  $(OBJS)


clean:
	rm -f craw $(OBJS)
