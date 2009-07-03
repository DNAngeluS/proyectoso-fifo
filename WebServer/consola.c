#include "consola.h"

extern void rutinaDeError(char *error);

extern ptrListaThread listaThread;
extern struct hashManager hashman;
extern int codop;
extern configuracion config;
extern int crawPresence;

void rutinaAtencionConsola (LPVOID args)
{
	/*Mientras no se ingrese la finalizacion*/
	while (codop != FINISH)
	{
		char consolaStdin[MAX_INPUT_CONSOLA];
		char *input;
		int centinela=0;

		memset(consolaStdin, '\0', sizeof(consolaStdin));

/**------	Validaciones a la hora de ingresar comandos	------**/

		gets_s(consolaStdin, MAX_INPUT_CONSOLA);
		
		/*Si esta el Crawler demorar todo ingreso de comandos*/
		if (crawPresence == 1)
		{
			struct timeval timeout = {2, 0};
			
			do 
				select(0, 0, 0, 0, &timeout);
			while (crawPresence == 1);
		}

		if (*consolaStdin == '-')
		{
			input = consolaStdin;
			if (!strcmp(input, "-queuestatus"))
			{
				printf("%s", STR_MSG_QUEUESTATUS);
				imprimeLista (listaThread);
            }

            else if (!strcmp(&consolaStdin[1], "run"))
            {
                 if (codop != RUNNING)
                 {
                    printf("%s", STR_MSG_RUN);
                    codop = RUNNING;
                 }
                 else
                     printf("%s", STR_MSG_INVALID_RUN);
            }

            else if (!strcmp(&consolaStdin[1], "finish"))
            {
                 printf("%s", STR_MSG_FINISH);
                 codop = FINISH;
            }

            else if (!strcmp(&consolaStdin[1], "outofservice"))
            {
                 if (codop != OUTOFSERVICE)
                 {
                    printf("%s", STR_MSG_OUTOFSERVICE);
                    codop = OUTOFSERVICE;
                 }
                 else
                     printf("%s", STR_MSG_INVALID_OUTOFSERVICE);
            }

			else if (!strncmp(&consolaStdin[1], "private", strlen("private")))
			{
				char *file = NULL, *next = NULL;

				file = strtok_s(consolaStdin, " ", &next);
				file = strtok_s(NULL, "\0", &next);
				
				if (!(file != NULL && isalnum(*file)))
					printf("%s", STR_MSG_INVALID_ARG);
				else
				{
					char path[MAX_PATH];
					DWORD attr;

					sprintf_s(path, MAX_PATH, "%s\\%s", config.directorioFiles, file);
					attr = GetFileAttributes(path);
					
					if (attr == 0)
						rutinaDeError("GetFile Attributes");
					else if (attr == INVALID_FILE_ATTRIBUTES)
						printf("%s", STR_MSG_INVALID_FILE);
					else if (attr == FILE_ATTRIBUTE_READONLY)
						printf("%s", STR_MSG_INVALID_PRIVATE);
					else
					{
						if (SetFileAttributes(path, FILE_ATTRIBUTE_READONLY) == 0)
							rutinaDeError("SetFileAttributes");
						printf("%s", STR_MSG_PRIVATE);
					}
				}
			}

			else if (!strncmp(&consolaStdin[1], "public", strlen("public")))
			{
				char *file = NULL, *next = NULL;

				file = strtok_s(consolaStdin, " ", &next);
				file = strtok_s(NULL, "\0", &next);
				
				if (!(file != NULL && isalnum(*file)))
					printf("%s", STR_MSG_INVALID_ARG);
				else
				{
					char path[MAX_PATH];
					DWORD attr;

					sprintf_s(path, MAX_PATH, "%s\\%s", config.directorioFiles, file);
					attr = GetFileAttributes(path);
					
					if (attr == 0)
						rutinaDeError("GetFile Attributes");
					else if (attr == INVALID_FILE_ATTRIBUTES)
						printf("%s", STR_MSG_INVALID_FILE);
					else if (attr == FILE_ATTRIBUTE_NORMAL)
						printf("%s", STR_MSG_INVALID_PUBLIC);
					else
					{
						if (SetFileAttributes(path, FILE_ATTRIBUTE_NORMAL) == 0)
							rutinaDeError("SetFileAttributes");
						printf("%s", STR_MSG_PUBLIC);
					}
				}
			}

			else if (!strcmp(&consolaStdin[1], "files"))
			{
				int i;
				printf("%s", STR_MSG_HASH);
				if (!hashVacia())
				{
					printf("%d archivos en Tabla Hash:\r\n", hashman.ocupados);
					for (i=0; i<HASHSIZE; i++)
						if (hashman.hashtab[i] != NULL)
							printf("\t%s : %s\r\n", hashman.hashtab[i]->md5, hashman.hashtab[i]->file);
				}
				else
					printf("(Vacia)\r\n");
				printf("\r\n");
			}
			else if (!strcmp(&consolaStdin[1], "reset"))
			{
				if (hashCleanAll() < 0)
					printf("%s", STR_MSG_INVALID_RESET);
				else
					printf("%s", STR_MSG_RESET);
			}

            else if (!strcmp(&consolaStdin[1], "help"))
			   printf("%s", STR_MSG_HELP);

            else
                printf("%s", STR_MSG_INVALID_INPUT);
		}
        else
            printf("%s", STR_MSG_INVALID_INPUT);
	 
	}
	_endthreadex(0);
}