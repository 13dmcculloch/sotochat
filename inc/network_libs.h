/* Network includes to speed things up */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_CONNS 20

#define MESSENGER_PATH "/tmp/chatmsg"  /* UNIX socket */
#define MESSENGER_ID 1

#define SHM_PATH "/tmp/chatsock"  /* IPs of clients */ 
#define SHM_KEY_ID 1

/* return -1 on error */
int create_tcp_server(char *client_ip, short port, int backlog);
int create_tcp_client(char *server_ip, short port);
