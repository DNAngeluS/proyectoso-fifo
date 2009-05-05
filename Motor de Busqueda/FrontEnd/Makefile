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


#Flags
CC = exec gcc
CFLAGS = -ansi -lsocket -lnsl -g

#Dependencias
OBJS = frontend.o irc.o http.o config.o

frontend: $(OBJS)
	$(CC) $(CFLAGS) -o frontend $(OBJS)

clean:
	rm -f frontend $(OBJS)
