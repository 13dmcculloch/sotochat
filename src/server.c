#include "server.h"

static int client_process(int client_fd);

int server(int port1, int port2)
{
    struct in_addr any_ip;
    any_ip.s_addr = INADDR_ANY;
    int server_fd = create_tcp_server(inet_ntoa(any_ip), port1, MAX_CONNS);

    struct sockaddr_in client_sa;
    socklen_t client_len;

    int client_fd;
    while((client_fd = accept(server_fd, (struct sockaddr *)NULL,
        NULL)) > 0)
    {
        pid_t client_pid;
        client_pid = fork();

        if(!client_pid) return client_process(client_fd);
    }

    return 0;
}

static int client_process(int client_fd)
{
    char msg_buf[MESSAGE_BUF_LEN];
    while(read(client_fd, msg_buf, sizeof msg_buf))
    {
        Message msg = deserialise_message(msg_buf);

        printf("%s: %s", msg.username, msg.message);
    }
    
    return 0;
}
