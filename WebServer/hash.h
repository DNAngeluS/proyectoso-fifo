#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <Wincrypt.h>

#define HASHSIZE 101
#define BUF_SIZE 1024
#define MD5LEN 16

struct nlist {
	struct nlist *next;
	char *file;
	char *md5;
};

unsigned hash					(char *s);
struct nlist *hashLookup		(char *s);
struct nlist *hashInstall		(char *file, char *md5);
int hashCleanAll				();
int hashClean					(char *file);
int hashMD5						(char *filename, char *md5sum);
int hashSave					();
int hashLoad					();
BOOL hashVacia					();