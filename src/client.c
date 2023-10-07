#include <unistd.h>

#include "network_libs.h"
#include "client.h"

int client(char *server_ip, short port1, short port2)
{
    Message msg;

    int server_fd = create_tcp_client(server_ip, port1);
    
    fputs("Username: ", stdout);
    fgets(msg.username, sizeof msg.username, stdin);
    msg.username[strcspn(msg.username, "\n")] = 0;

    char msg_buf[MESSAGE_LEN];
    char send_buf[MESSAGE_BUF_LEN];
    while(1)
    {
        printf("Message: ");
        fgets(msg_buf, sizeof msg_buf, stdin);

        msg.message_len = strlen(msg_buf);
        memcpy(msg.message, msg_buf, strlen(msg_buf) + 1);

        if(serialise_message(&msg, send_buf) < 0)
        {
            fputs("Error serialising message.\n", stderr);
            return 1;
        }

        write(server_fd, send_buf, sizeof send_buf);
    }

    return 0;
}
