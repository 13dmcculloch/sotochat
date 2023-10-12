#include "server.h"

#ifndef DEBUG
#define DEBUG

static int client_process(int client_fd);
static int message_process(short port2);

static struct in_addr *shm_create();
static struct in_addr *shm_attach();

static int clients_close(int *client_fd);

static void clients_list(const int *client_fd);
static void addr_list(const struct in_addr *addr_p);

static void sort_child(int signal);

int server(int port1, int port2)
{
    struct in_addr *addr_p = shm_create();
    if(addr_p == NULL)
    {
        fputs("server: error creating shared memory\n", stderr);
        return 1;
    }

    signal(SIGCHLD, sort_child);

    pid_t message_pid = fork();
    if(!message_pid) return message_process(port2);

    struct in_addr any_ip;
    any_ip.s_addr = INADDR_ANY;
    int server_fd = create_tcp_server(inet_ntoa(any_ip), port1, MAX_CONNS);

    int client_fd;
    while((client_fd = accept(server_fd, (struct sockaddr *)NULL,
        NULL)) > 0)
    {
        signal(SIGCHLD, sort_child);

        pid_t client_pid = fork();

        if(!client_pid) 
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

            #ifdef DEBUG
            addr_list(addr_p);
            #endif

            return client_process(client_fd);
        }
    }

    return 0;
}

static int client_process(int client_fd)
{
    struct sockaddr_un message_sa;
    message_sa.sun_family = AF_UNIX;
    strcpy(message_sa.sun_path, MESSENGER_PATH);

    int message_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(message_fd < 0)
    {
        perror("client_process: socket");
        return 1;
    }

    if(connect(message_fd, (struct sockaddr *)&message_sa, 
        sizeof message_sa) < 0)
    {
        perror("client_process: connect");
        return 1;
    }

    char msg_buf[MESSAGE_BUF_LEN];
    while(read(client_fd, msg_buf, sizeof msg_buf))
    {
        Message msg = deserialise_message(msg_buf);

        printf("[GLOBAL]: %s: %s", msg.username, msg.message);
        
        if(write(message_fd, msg_buf, sizeof msg_buf) < 0)
        {
            perror("client_process: write");
            return 1;
        }
    } 

    return 0;
}

static int message_process(short port2)
{
    struct in_addr *addr_p = shm_attach();
    if(addr_p == NULL)
    {
        fputs("message_process: error attaching shared memory\n", stderr);
        return 1;
    }

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
    memset(client_fd, -1, sizeof client_fd);

    char msg_buf[MESSAGE_BUF_LEN];
    while(recv(message_fd, msg_buf, sizeof msg_buf, 0) > 0)
    {
        int i;
        for(i = 0; i < MAX_CONNS; ++i)
        {
            /* open new socket if client_fd is invalid */
            if(client_fd[i] < 0)
            {
                if(!addr_p[i].s_addr) continue;
                client_fd[i] = create_tcp_client(inet_ntoa(addr_p[i]), 
                    port2);
                break;
            }
        }
        /* handle closed sockets here (no longer in shm) */

        #ifdef DEBUG
        clients_list(client_fd);
        #endif

        /* loop through and write to open sockets */
        for(int j = 0; j < MAX_CONNS; ++j)
        {
            if(client_fd[j] < 0) continue;

            if(write(client_fd[j], msg_buf, sizeof msg_buf) < 0)
            {
                perror("message_process: write");
                return 1;
            }
        }
    }

    return 0;
}

static struct in_addr *shm_create()
{
    static struct in_addr *addr_p = NULL;

    key_t messenger_key = ftok(SHM_PATH, SHM_KEY_ID);

    int messenger_shmid = shmget(messenger_key, 
        (sizeof(struct in_addr[MAX_CONNS])), IPC_CREAT | S_IWUSR | S_IRUSR);
    if(messenger_shmid < 0)
    {
        perror("server: shmget");
        return addr_p;
    }

    addr_p = (struct in_addr *)shmat(messenger_shmid, NULL, 0);
    if((void *)addr_p == (void *)-1)
    {
        perror("server: shmat");
        return addr_p;
    }

    memset(addr_p, 0, MAX_CONNS * sizeof(struct in_addr));

    return addr_p;
}

static struct in_addr *shm_attach()
{
    static struct in_addr *addr_p = NULL;

    key_t messenger_key = ftok(SHM_PATH, SHM_KEY_ID);

    int messenger_shmid = shmget(messenger_key, 
        (sizeof(struct in_addr[MAX_CONNS])), S_IWUSR | S_IRUSR);
    if(messenger_shmid < 0)
    {
        perror("message_process: shmget");
        return addr_p;
    }
    addr_p = (struct in_addr *)shmat(messenger_shmid, NULL, 0);
    if((void *)addr_p == (void *)-1)
    {
        perror("message_process: shmat");
        return addr_p;
    }

    return addr_p;
}

static int clients_close(int *client_fd)
{
    for(int i = 0; i < MAX_CONNS; ++i)
    {
        if(close(client_fd[i]))
        {
            perror("clients_close: close");
            return 1;
        }
    }

    return 0;
}

static void clients_list(const int *client_fd)
{
    fputs("Clients: ", stderr);
    for(int i = 0; i < MAX_CONNS; ++i)
        fprintf(stderr, "%d ", client_fd[i]);
    fputs("\n", stderr);
}

static void addr_list(const struct in_addr *addr_p)
{
    fputs("Addresses:", stderr);
    for(int i = 0; i < MAX_CONNS; ++i)
        fprintf(stderr, "%d ", addr_p[i].s_addr);
    fputs("\n", stderr);
}

static void sort_child(int signal)
{
    wait(NULL);
}

#endif
