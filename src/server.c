/* Server function definition.
Douglas McCulloch, October 2023
*/

#include "server.h"

#define DEBUG

static int client_process_w(int server_fd, struct in_addr *addr_p);

/* return -1 on error, otherwise in_addr of disconnected client */
static struct in_addr client_process(int client_fd);

/* deal with address of connected client */
static int addr_add(int client_fd, struct in_addr *addr_p);
static int addr_remove(struct in_addr last_addr, struct in_addr *addr_p);

/* process to send last message to all clients */
static int message_process(short port2);
static int sockets_check(int client_fd[MAX_CONNS], struct in_addr *addr_p,
    short port2);
static int sockets_write(int client_fd[MAX_CONNS], 
    char msg_buf[MESSAGE_BUF_LEN]);
    
static void sort_child(int signal);

/* store addresses of connected clients */
static struct in_addr *shm_addr_create();
static struct in_addr *shm_addr_attach();

int server(int port1, int port2)
{
    struct in_addr *addr_p = shm_addr_create();

    /* signals */
    signal(SIGCHLD, sort_child);

    if(!fork()) return message_process(port2);

    struct in_addr any_ip;
    any_ip.s_addr = INADDR_ANY;
    int server_fd = create_tcp_server(inet_ntoa(any_ip), port1, MAX_CONNS);

    return client_process_w(server_fd, addr_p);
}

static int client_process_w(int server_fd, struct in_addr *addr_p)
{
    int client_fd;
    while((client_fd = accept(server_fd, (struct sockaddr *)NULL,
        NULL)) > 0)
    {
        pid_t client_pid;
        client_pid = fork();

        if(!client_pid) 
        {
            int rc;
            if((rc = addr_add(client_fd, addr_p))) return rc;

            /* enter client process handler */
            struct in_addr last_addr;
            if((last_addr.s_addr = client_process(client_fd).s_addr) < 0)
            {
                fputs("server: client process error.\n", stderr);
                return 1;
            }
            
            if((rc = addr_remove(last_addr, addr_p))) return rc;
        }
    }
    
    return 0;
}

static struct in_addr client_process(int client_fd)
{
    struct in_addr return_addr;
    return_addr.s_addr = -1;

    struct sockaddr_un message_sa;
    message_sa.sun_family = AF_UNIX;
    strcpy(message_sa.sun_path, MESSENGER_PATH);

    int message_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(message_fd < 0)
    {
        perror("client_process: socket");
        return return_addr;
    }

    if(connect(message_fd, (struct sockaddr *)&message_sa, 
        sizeof message_sa) < 0)
    {
        perror("client_process: connect");
        return return_addr;
    }

    struct sockaddr_in client_sa;
    socklen_t client_len = sizeof client_sa;
    if(getpeername(client_fd, (struct sockaddr *)&client_sa, &client_len) < 0)
    {
        perror("client_process: getpeername");
        return return_addr;
    }

    printf("Incoming connection from %s\n", inet_ntoa(client_sa.sin_addr));

    char msg_buf[MESSAGE_BUF_LEN];
    while(read(client_fd, msg_buf, sizeof msg_buf))
    {
        Message msg = deserialise_message(msg_buf);

        printf("[GLOBAL]: %s: %s", msg.username, msg.message);
        
        if(write(message_fd, msg_buf, sizeof msg_buf) < 0)
        {
            perror("client_process: write");
            return return_addr;
        }
    } 

    printf("%s disconnected.\n", inet_ntoa(client_sa.sin_addr));

    close(client_fd);

    return client_sa.sin_addr;
}

static int addr_add(int client_fd, struct in_addr *addr_p)
{
    /* add address of client to addr_p */
    struct sockaddr_in client_sa;
    socklen_t client_len;

    if(getpeername(client_fd, (struct sockaddr *)&client_sa, 
        &client_len) < 0)
    {
        perror("server: getpeername");
        return 1;
    }

    for(int i = 0; i < MAX_CONNS; ++i)
    {
        if(addr_p[i].s_addr == 0)
        {
            addr_p[i].s_addr = client_sa.sin_addr.s_addr;
            break;
        }
    }
    
    return 0;
}

static int addr_remove(struct in_addr last_addr, struct in_addr *addr_p)
{
    for(int i = 0; i < MAX_CONNS; ++i)
    {
        if(addr_p[i].s_addr == last_addr.s_addr)
        {
            addr_p[i].s_addr = 0;
            break;
        }
    }

    return 0;
}

static int message_process(short port2)
{
    struct in_addr *addr_p = shm_addr_attach();

    /* UNIX socket */
    /* acts like a server. Must bind to local address */
    struct sockaddr_un message_sa;
    
    unlink(MESSENGER_PATH);

    message_sa.sun_family = AF_UNIX;
    strcpy(message_sa.sun_path, MESSENGER_PATH);
    
    int message_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(message_fd < 0)
    {
        perror("message_process: socket");
        return 1;
    }

    if(bind(message_fd, (struct sockaddr *)&message_sa, 
        sizeof message_sa) < 0)
    {
        perror("message_process: bind");
        return 1;
    }

    /* array of open client sockets */
    int client_fd[MAX_CONNS];  
    memset(client_fd, 0, sizeof client_fd);

    /* send back to clients */
    int rc;
    char msg_buf[MESSAGE_BUF_LEN];
    while(recv(message_fd, msg_buf, sizeof msg_buf, 0) > 0)
    {
        if((rc = sockets_check(client_fd, addr_p, port2))) return rc;
        if((rc = sockets_write(client_fd, msg_buf))) return rc;
    }

    return 0;
}

static int sockets_check(int client_fd[MAX_CONNS], struct in_addr *addr_p,
    short port2)
{
    for(int i = 0; i < MAX_CONNS; ++i)
    {
        if(!client_fd[i])  /* socket open */
        {
            /* open socket? */
            if(!addr_p[i].s_addr) continue;

            if((client_fd[i] = create_tcp_client(inet_ntoa(addr_p[i]), 
                port2) < 0)) return 1;
            break;
        }
        else  /* socket closed */
        {
            /* close socket? */
            if(!addr_p[i].s_addr)
            {
                if(close(client_fd[i]) < 0)
                {
                    perror("sockets_check: close");
                    return 1;
                }

                client_fd[i] = 0;
            }
        }

    }

    return 0;
}

static int sockets_write(int client_fd[MAX_CONNS], 
    char msg_buf[MESSAGE_BUF_LEN])
{
    for(int j = 0; j < MAX_CONNS; ++j)
    {
        if(client_fd[j] == 0) continue;

        if(write(client_fd[j], msg_buf, 
            MESSAGE_BUF_LEN * sizeof *msg_buf) < 0)
        {
            perror("sockets_write: write");
            return 1;
        }

        #ifdef DEBUG
        fputs("Sent to client.\n", stderr);
        #endif
    }

    return 0;
}

static void sort_child(int signal)
{
    wait(NULL);
}

static struct in_addr *shm_addr_create()
{
    struct in_addr *ret_addr = NULL;

    /* shared memory */
    key_t messenger_key = ftok(SHM_PATH, SHM_KEY_ID);

    int messenger_shmid = shmget(messenger_key, 
        (sizeof(struct in_addr[MAX_CONNS])), IPC_CREAT | S_IWUSR | S_IRUSR);
    if(messenger_shmid < 0)
    {
        perror("server: shmget");
        return ret_addr;
    }
    struct in_addr *addr_p = (struct in_addr *)shmat(messenger_shmid, NULL, 0);
    if((void *)addr_p == (void *)-1)
    {
        perror("server: shmat");
        return ret_addr;
    }

    memset(addr_p, 0, MAX_CONNS);

    return addr_p;
}

static struct in_addr *shm_addr_attach()
{
    struct in_addr *ret_addr = NULL;

    key_t messenger_key = ftok(SHM_PATH, SHM_KEY_ID);

    int messenger_shmid = shmget(messenger_key, 
        (sizeof(struct in_addr[MAX_CONNS])), S_IWUSR | S_IRUSR);
    if(messenger_shmid < 0)
    {
        perror("message_process: shmget");
        return ret_addr;
    }
    struct in_addr *addr_p = 
        (struct in_addr *)shmat(messenger_shmid, NULL, 0);
    if((void *)addr_p == (void *)-1)
    {
        perror("message_process: shmat");
        return ret_addr;
    }

    return addr_p;
}

