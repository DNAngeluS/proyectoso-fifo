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


#Flags
CC = exec gcc-4.1
CFLAGS = -g  -ansi# -lpthread

#Dependencias
OBJS = webstore.o config.o

webstore: $(OBJS)
	$(CC) $(CFLAGS) -o webstore  $(OBJS)

clean:
	rm -f webstore $(OBJS)