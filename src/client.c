/* Client function definition.
Douglas McCulloch, October 2023
*/

#include <unistd.h>

#include "network_libs.h"
#include "client.h"

static int messages_send(int server_fd, Message msg);
static int exceed_check(int *exceed_f);
static int length_check(char msg_buf[MESSAGE_LEN], int *exceed_f);

static int messages_receive(char *server_ip, short port2);

int client(char *server_ip, short port1, short port2)
{
    Message msg;

    int server_fd = create_tcp_client(server_ip, port1);
    
    fputs("Username: ", stdout);
    fgets(msg.username, sizeof msg.username, stdin);
    msg.username[strcspn(msg.username, "\n")] = 0;

    if(!fork()) return messages_receive(server_ip, port2);

    while(!messages_send(server_fd, msg));

    shutdown(server_fd, SHUT_RDWR);

    return 0;
}

static int messages_send(int server_fd, Message msg)
{
    char msg_buf[MESSAGE_LEN];
    char send_buf[MESSAGE_BUF_LEN];

    static int exceed_f = 0;
    if(exceed_check(&exceed_f)) return 0;

    fputs("Message: ", stdout);
    fgets(msg_buf, sizeof msg_buf, stdin);
    if(length_check(msg_buf, &exceed_f)) return 0;

    if(!strcmp(msg_buf, "exit\n")) return 1;

    /* populate msg */
    msg.message_len = strlen(msg_buf);
    memcpy(msg.message, msg_buf, strlen(msg_buf) + 1);

    if(serialise_message(&msg, send_buf))
    {
        fputs("Error serialising message.\n", stderr);
        return 1;
    }

    write(server_fd, send_buf, sizeof send_buf);

    return 0;
}

static int exceed_check(int *exceed_f)
{
    if(*exceed_f)
    {
        while(fgetc(stdin) != '\n');
        *exceed_f = 0;
        return 1;
    }

    return 0;
}

static int length_check(char msg_buf[MESSAGE_LEN], int *exceed_f)
{
    char f = msg_buf[strlen(msg_buf) - 1];
    if(f != '\n')
    {
        fputs("Message length exceeded! (Max 1024 C)\n", stderr);
        *exceed_f = 1;
        return 1;
    }

    return 0;
}

static int messages_receive(char *server_ip, short port2)
{
    int server_fd = create_tcp_server(server_ip, port2, 1);

    int client_fd = accept(server_fd, (struct sockaddr *)NULL, NULL);

    if(client_fd < 0)
    {
        perror("messages_receive: accept");
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
