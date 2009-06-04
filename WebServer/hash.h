#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <Wincrypt.h>

#define HASHSIZE 101
#define BUFSIZE 1024
#define MD5LEN 16

struct nlist {
	struct nlist *next;
	char *file;
	char *md5;
};

unsigned hash					(char *s);
struct nlist *hashLookup		(/*struct nlist **hashtab, */char *s);
struct nlist *hashInstall		(/*struct nlist **hashtab, */char *file, char *md5);
int hashClean					(/*struct nlist **hashtab, */char *file);
int hashMD5						(char *filename, char *md5sum);
int hashSave					(char *tmpFile);
int hashLoad					();
BOOL hashVacia					();