#include <unistd.h>

#include "network_libs.h"
#include "client.h"

static int receive_messages(char *server_ip, short port2);

static void sort_child(int signal);

int client(char *server_ip, short port1, short port2)
{
    Message msg;

    int server_fd = create_tcp_client(server_ip, port1);

    signal(SIGCHLD, sort_child);

    pid_t messages_pid = fork();
    if(!messages_pid) return receive_messages(server_ip, port2);
    
    fputs("Username: ", stdout);
    fgets(msg.username, sizeof msg.username, stdin);
    msg.username[strcspn(msg.username, "\n")] = 0;

    char msg_buf[MESSAGE_LEN];
    char send_buf[MESSAGE_BUF_LEN];
    while(1)
    {
        fgets(msg_buf, sizeof msg_buf, stdin);

        msg.message_len = strlen(msg_buf);
        memcpy(msg.message, msg_buf, strlen(msg_buf) + 1);

        if(serialise_message(&msg, send_buf))
        {
            fputs("Error serialising message.\n", stderr);
            return 1;
        }

        write(server_fd, send_buf, sizeof send_buf);
    }

    return 0;
}

static int receive_messages(char *server_ip, short port2)
{
    int server_fd = create_tcp_server(server_ip, port2, 1);

    int client_fd = accept(server_fd, (struct sockaddr *)NULL, NULL);
    if(client_fd < 0)
    {
        perror("receive_messages: accept");
        return 1;
    }

    char recv_buf[MESSAGE_BUF_LEN];
    while(read(client_fd, recv_buf, sizeof recv_buf))
    {
        Message msg = deserialise_message(recv_buf);

        printf("%s: %s", msg.username, msg.message);
    }
    
    return 0;
}

static void sort_child(int signal)
{
    wait(NULL);
}
