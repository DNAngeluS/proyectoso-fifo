#include "hash.h"

extern struct hashManager hashman;
static char *miStrdup(char *s);

unsigned hash(char *s)
{
	unsigned hashval;

	for (hashval = 0; *s != '\0'; s++)
		hashval = *s + 31 * hashval;
	return hashval % HASHSIZE;
}

struct nlist *hashLookup(char *s)
{
	struct nlist *np;

	for (np = hashman.hashtab[hash(s)]; np != NULL; np = np->next)
		if (strcmp(s, np->file) == 0)
			return np;	/*Se encontro*/
	return NULL;	/*No se encontro*/
}

struct nlist *hashInstall(char *file, char *md5)
{
	struct nlist *np;
	unsigned hashval;

	if ((np = hashLookup(file)) == NULL)
	/*No fue encontrado*/
	{
		np = HeapAlloc(GetProcessHeap(), 0, sizeof(*np));
		if (np == NULL || (np->file = miStrdup(file)) == NULL)
			return NULL;
		hashval = hash(file);
		np->next = hashman.hashtab[hashval];
		hashman.hashtab[hashval] = np;

		/*Actualiza estado del Hash Manager*/
		hashman.flag[hashval] = RECIENTEMENTE_ACCEDIDO;
		hashman.ocupados++;
	}
	else /*Ya esta alli*/
		HeapFree (GetProcessHeap(), 0, np->md5); /*Libera anterior md5*/
	if ((np->md5 = miStrdup(md5)) == NULL)
		return NULL;
	return np;
}

int hashCleanAll()
{
	int i;

	for (i = 0; i < HASHSIZE; i++)
		if (hashman.hashtab[i] != NULL)
			if (hashClean(hashman.hashtab[i]->file) < 0)
				return -1;
	return 0;
}

int hashClean(char *file)
{
    struct nlist *np1, *np2;
	int index = hash(file);

    if ((np1 = hashLookup(file)) == NULL)	/*No encontro*/
        return 1;

    for ( np1 = np2 = hashman.hashtab[index]; np1 != NULL; np2 = np1, np1 = np1->next ) 
	{
        if ( strcmp(file, np1->file) == 0 ) 
		/*Encontro*/
		{
            /*Remover nodo de la lista*/
            if ( np1 == np2 )
                hashman.hashtab[index] = np1->next;
            else
                np2->next = np1->next;
			
			/*Actualizar estado del Hash Manager*/
			hashman.flag[index] = VACIO;
			hashman.ocupados--;

            /*Liberar memoria*/
			HeapFree (GetProcessHeap(), 0, np1->file);
			HeapFree (GetProcessHeap(), 0, np1->md5);
			HeapFree (GetProcessHeap(), 0, np1);

            return 0;
        }
    }

    return 1;  /*No encontro*/
}

int hashMD5(char *filename, char *dir, char *md5sum)
{
    DWORD dwStatus = 0;
    BOOL bResult = FALSE;
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HANDLE hFile = NULL;
    BYTE rgbFile[BUF_SIZE];
    DWORD cbRead = 0;
    BYTE rgbHash[MD5LEN];
    DWORD cbHash = 0;
    CHAR rgbDigits[] = "0123456789abcdef";
	char fileAndDir[MAX_PATH];

	memset(md5sum,'\0', sizeof(*md5sum));
	memset(fileAndDir, '\0', sizeof(fileAndDir));

	wsprintf(fileAndDir, "%s\\%s", dir, filename);

    hFile = CreateFile(fileAndDir,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwStatus = GetLastError();
        printf("Error opening file %s\nError: %d\r\n", filename, 
			dwStatus); 
        return dwStatus;
    }

    if (!CryptAcquireContext(&hProv,
        NULL,
        NULL,
        PROV_RSA_FULL,
        CRYPT_VERIFYCONTEXT))
    {
        dwStatus = GetLastError();
        printf("CryptAcquireContext failed: %d\r\n", dwStatus); 
        CloseHandle(hFile);
        return dwStatus;
    }

    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
    {
        dwStatus = GetLastError();
        printf("CryptAcquireContext failed: %d\r\n", dwStatus); 
        CloseHandle(hFile);
        CryptReleaseContext(hProv, 0);
        return dwStatus;
    }

    while (bResult = ReadFile(hFile, rgbFile, BUF_SIZE, 
		&cbRead, NULL))
    {
        if (0 == cbRead)
        {
            break;
        }

        if (!CryptHashData(hHash, rgbFile, cbRead, 0))
        {
            dwStatus = GetLastError();
            printf("CryptHashData failed: %d\r\n", dwStatus); 
            CryptReleaseContext(hProv, 0);
            CryptDestroyHash(hHash);
            CloseHandle(hFile);
            return dwStatus;
        }
    }

    if (!bResult)
    {
        dwStatus = GetLastError();
        printf("ReadFile failed: %d\r\n", dwStatus); 
        CryptReleaseContext(hProv, 0);
        CryptDestroyHash(hHash);
        CloseHandle(hFile);
        return dwStatus;
    }

    cbHash = MD5LEN;
    if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
    {
        DWORD i;
        
        for (i = 0; i < cbHash; i++)
        {
            char buf[5];
            sprintf_s(buf, sizeof(buf), "%c%c", rgbDigits[rgbHash[i] >> 4],
                rgbDigits[rgbHash[i] & 0xf]);
            lstrcat(md5sum, buf);
        }
    }
    else
    {
        dwStatus = GetLastError();
        printf("CryptGetHashParam failed: %d\r\n", dwStatus); 
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    CloseHandle(hFile);

    return dwStatus; 
}

int hashSave()
{
	DWORD buf_size, temp;
    HANDLE archivoHash;
	char buf[BUF_SIZE];
	char line[MAX_PATH];
	DWORD lim;
	int i;
	
    archivoHash = CreateFileA("hash.txt", GENERIC_WRITE, 0, NULL,
                             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (archivoHash == INVALID_HANDLE_VALUE) 
    {
            printf("No se pudo crear el archivo hash: "
                                    "(error %d)\r\n", GetLastError());       
			return -1;
    }

	if (hashVacia())
		return 0;

	memset(buf, '\0', BUF_SIZE);
	memset(line, '\0', MAX_PATH);
	for (i = lim = 0; i < HASHSIZE; i++)
	{	
        if (hashman.hashtab[i] != NULL)
		{
			sprintf_s(line, MAX_PATH, "%s\\%s/", hashman.hashtab[i]->file, hashman.hashtab[i]->md5);
			lstrcat(buf, line);
			lim = (DWORD) lstrlen(buf);
			memset(line, '\0', MAX_PATH);
		}
	}

	temp = WriteFile(archivoHash, buf, lim, &buf_size, NULL);
    CloseHandle(archivoHash);

	printf("Se a guardado Tabla Hash.\r\n");

	return 0;
}

int hashLoad()
{
	DWORD buf_size, temp;
    HANDLE archivoHash;
	char *lim, *primero, *act, *file, *md5, *pos;
	char buf[BUF_SIZE];

    memset(hashman.hashtab, '\0', sizeof(hashman.hashtab));
	memset(hashman.flag, VACIO, sizeof(hashman.flag));
	hashman.ocupados = 0;

    archivoHash = CreateFileA("hash.txt", GENERIC_READ, 0, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (archivoHash == INVALID_HANDLE_VALUE) 
    {
            if (GetLastError() == 2)
            {
               printf("No se encontro ninguna Tabla Hash. Se carga la tabla vacia.\r\n\r\n");
               return 0;
            }
            else
                printf("No se pudo abrir el archivo hash: "
                                    "(error %d)\r\n", GetLastError());       
			return -1;
    }
    
	temp = ReadFile(archivoHash, buf, BUF_SIZE, &buf_size, NULL);
    CloseHandle(archivoHash);
    if (temp == FALSE) 
    {
            printf("No se pudo leer el archivo hash: "
                                    "(error %d)\r\n", GetLastError());
            return -1;
    }
	
	lim = buf + buf_size;
	file = md5 = pos = NULL;
	for (primero = act = buf; act < lim; act++) 
	{
        switch (*act) 
		{
			case '\\':
				*act = '\0';
				file = primero;
				md5 = act + 1;
				break;
			case '/':
				primero = act + 1; 
				*act = '\0';
				if (file != NULL && md5 != NULL) 
                    hashInstall(file, md5);
				file = md5 = pos = NULL;
				break;
		}
	}
	printf("Se a cargado la Tabla Hash.\r\n\r\n");
	
	return 0;
}

static int ingresarValor(char *file, char *md5, int pos)
{
	hashman.hashtab[pos]->file = miStrdup(file);
	hashman.hashtab[pos]->md5 = miStrdup(md5);

	return 0;
}

BOOL hashVacia()
{
	int i=0;

	while (i < HASHSIZE)
	{
		if (hashman.hashtab[i++] != NULL)
			break;
	}

	return (i == HASHSIZE);
}


char *miStrdup(char *s)
{
	char *result = HeapAlloc(GetProcessHeap(), 0, strlen(s) + 1);
    
	if (result == NULL)
		return NULL;

    lstrcpy(result, s);
    return result;
}
