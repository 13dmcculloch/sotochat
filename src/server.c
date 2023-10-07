#include "server.h"

/* return -1 on error, otherwise in_addr of disconnected client */
static struct in_addr client_process(int client_fd);

static int message_process(short port2);

static void sort_child(int signal);

int server(int port1, int port2)
{
    /* shared memory */
    key_t messenger_key = ftok(SHM_PATH, SHM_KEY_ID);

    int messenger_shmid = shmget(messenger_key, 
        (sizeof(struct in_addr[MAX_CONNS])), IPC_CREAT | S_IWUSR | S_IRUSR);
    if(messenger_shmid < 0)
    {
        perror("server: shmget");
        return 1;
    }
    struct in_addr *addr_p = (struct in_addr *)shmat(messenger_shmid, NULL, 0);
    if((void *)addr_p == (void *)-1)
    {
        perror("server: shmat");
        return 1;
    }

    memset(addr_p, 0, MAX_CONNS);

    /* signals */
    signal(SIGCHLD, sort_child);

    if(!fork()) return message_process(port2);

    struct in_addr any_ip;
    any_ip.s_addr = INADDR_ANY;
    int server_fd = create_tcp_server(inet_ntoa(any_ip), port1, MAX_CONNS);

    int client_fd;
    while((client_fd = accept(server_fd, (struct sockaddr *)NULL,
        NULL)) > 0)
    {
        pid_t client_pid;
        client_pid = fork();

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

            /* enter client process handler */
            struct in_addr last_addr;
            if((last_addr.s_addr = client_process(client_fd).s_addr) < 0)
            {
                fputs("Error quitting client process.\n", stderr);
                return 1;
            }

            /* remove address of disconnected client */
            for(int i = 0; i < MAX_CONNS; ++i)
            {
                if(addr_p[i].s_addr == last_addr.s_addr)
                {
                    addr_p[i].s_addr = 0;
                    break;
                }
            }
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

static int message_process(short port2)
{
    /* shared memory */
    key_t messenger_key = ftok(SHM_PATH, SHM_KEY_ID);

    int messenger_shmid = shmget(messenger_key, 
        (sizeof(struct in_addr[MAX_CONNS])), S_IWUSR | S_IRUSR);
    if(messenger_shmid < 0)
    {
        perror("message_process: shmget");
        return 1;
    }
    struct in_addr *addr_p = 
        (struct in_addr *)shmat(messenger_shmid, NULL, 0);
    if((void *)addr_p == (void *)-1)
    {
        perror("message_process: shmat");
        return 1;
    }

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

    char msg_buf[MESSAGE_BUF_LEN];
    while(recv(message_fd, msg_buf, sizeof msg_buf, 0) > 0)
    {
        /* check sockets */
        int i;
        for(i = 0; i < MAX_CONNS; ++i)
        {
            if(!client_fd[i])  /* socket open */
            {
                /* open socket? */
                if(!addr_p[i].s_addr) continue;

                client_fd[i] = create_tcp_client(inet_ntoa(addr_p[i]), 
                    port2);
                break;
            }
            else  /* socket closed */
            {
                /* close socket? */
                if(!addr_p[i].s_addr)
                {
                    close(client_fd[i]);
                    client_fd[i] = 0;
                }
            }

        }

        /* write to sockets */
        for(int j = 0; j < MAX_CONNS; ++j)
        {
            if(client_fd[j] == 0) continue;
            write(client_fd[j], msg_buf, sizeof msg_buf);
        }
    }

    return 0;
}

static void sort_child(int signal)
{
    wait(NULL);
}
