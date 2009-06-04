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

unsigned hash(char *s);
struct nlist *lookup(/*struct nlist **hashtab, */char *s);
struct nlist *install(/*struct nlist **hashtab, */char *file, char *md5);
int clean(/*struct nlist **hashtab, */char *file);
int hashMD5(char *filename, char *md5sum);
int hashSave(char *tmpFile);
int hastLoad();