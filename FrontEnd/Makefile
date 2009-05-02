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
CC = exec gcc-4.1
CFLAGS = -ansi

#Dependencias
OBJS = frontend.o irc.o http.o config.o

frontend: $(OBJS)
	$(CC) $(CFLAGS) -o pred $(OBJS)

clean:
	rm -f frontend $(OBJS)
