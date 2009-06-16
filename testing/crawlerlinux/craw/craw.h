#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/timeb.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#define SOCKET int
#define SOCKADDR struct sockaddr
#define SOCKADDR_IN struct sockaddr_in
#define INVALID_SOCKET -1
#define MAX_PATH 256
#define SOCKET_ERROR -1
#define BUF_SIZE 1024
#define MAX_HTMLCODE 4096
