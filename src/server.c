#include "server.h"

#ifndef DEBUG
#define DEBUG

static int client_process(int client_fd);
static int message_process();

static void clients_list(const int *client_fd);

int server(int port1, int port2)
{
    /* shared memory */
    key_t messenger_key = ftok(SHM_PATH, SHM_KEY_ID);

    int messenger_shmid = shmget(messenger_key, 
        (sizeof(int) * MAX_CONNS), IPC_CREAT | S_IWUSR | S_IRUSR);
    if(messenger_shmid < 0)
    {
        perror("server: shmget");
        return 1;
    }
    int *client_p = (int *)shmat(messenger_shmid, NULL, 0);
    if((void *)client_p == (void *)-1)
    {
        perror("server: shmat");
        return 1;
    }

    /* empty client file descriptor must be -1 as 0 is valid */
    memset(client_p, -1, MAX_CONNS * sizeof(int));

    #ifdef DEBUG
    clients_list(client_p);
    #endif

    if(!fork()) return message_process();

    struct in_addr any_ip;
    any_ip.s_addr = INADDR_ANY;
    int server_fd = create_tcp_server(inet_ntoa(any_ip), port1, MAX_CONNS);

    int client_fd;
    while((client_fd = accept(server_fd, (struct sockaddr *)NULL,
        NULL)) > 0)
    {
        #ifdef DEBUG
        fprintf(stderr, "server_fd: %d\n", server_fd);
        fprintf(stderr, "client_fd: %d\n", client_fd);
        #endif

        pid_t client_pid;
        client_pid = fork();

        if(!client_pid) 
        {
            /* get address of client for use in create_tcp_client */
            struct sockaddr_in client_sa;
            socklen_t client_len = sizeof client_sa;

            if(getpeername(client_fd, (struct sockaddr *)&client_sa,
                &client_len) < 0)
            {
                perror("server: getpeername");
                return 1;
            }

            struct in_addr client_addr = client_sa.sin_addr;

            /* a socket connecting to client's message display process */
            int message_socket = create_tcp_client(
                inet_ntoa(client_addr), port2);
            if(message_socket < 0)
            {
                fputs("server: error creating client message socket\n",
                stderr);
                return 1;
            }

            /* add socket to shared memory */
            for(int i = 0; i < MAX_CONNS; ++i)
            {
                /* case of empty entry */
                if(client_p[i] < 0)
                {
                    /* this might be illegal... */
                    client_p[i] = message_socket;
                    break;
                }
            }

            #ifdef DEBUG
            clients_list(client_p);
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

static int message_process()
{
    /* shared memory - attach */
    key_t messenger_key = ftok(SHM_PATH, SHM_KEY_ID);

    int messenger_shmid = shmget(messenger_key, 
        (sizeof(int) * MAX_CONNS), S_IWUSR | S_IRUSR);
    if(messenger_shmid < 0)
    {
        perror("message_process: shmget");
        return 1;
    }
    int *client_p = 
        (int *)shmat(messenger_shmid, NULL, 0);
    if((void *)client_p == (void *)-1)
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

    #ifdef DEBUG
    fprintf(stderr, "message_fd: %d\n", message_fd);
    #endif

    char msg_buf[MESSAGE_BUF_LEN];
    while(recv(message_fd, msg_buf, sizeof msg_buf, 0) > 0)
    {
        #ifdef DEBUG
        clients_list(client_p);
        #endif

        /* loop through and write to open sockets */
        for(int j = 0; j < MAX_CONNS; ++j)
        {
            int fd = client_p[j];

            if(fd < 0) continue;

            #ifdef DEBUG
            fprintf(stderr, "Writing to %d\n", fd);
            #endif

            if(write(fd, msg_buf, sizeof msg_buf))
            {
                perror("message_process: write");
                return 1;
            }
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

#endif
