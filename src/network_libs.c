#include "network_libs.h"  

int create_tcp_server(char *client_ip, short port, int backlog)
{
    struct sockaddr_in server_sa;

    server_sa.sin_family = AF_INET;
    server_sa.sin_port = htons(port);
    server_sa.sin_addr.s_addr = inet_addr(client_ip);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(bind(server_fd, (struct sockaddr *) &server_sa, sizeof server_sa) < 0)
    {
        perror("create_tcp_server: bind");
        return -1;
    }

    if(listen(server_fd, backlog) < 0)
    {
        perror("create_tcp_server: listen");
        return -1;
    }

    /* it blocks on accept */
    return server_fd;
}

int create_tcp_client(char *server_ip, short port)
{
    struct sockaddr_in server_sa;

    server_sa.sin_family = AF_INET;
    server_sa.sin_port = htons(port);
    server_sa.sin_addr.s_addr = inet_addr(server_ip);
    if(server_sa.sin_addr.s_addr == INADDR_ANY) 
    {
        fputs("create_tcp_client: cannot bind to INADDR_ANY\n", stderr);
        return -1;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(connect(server_fd, (struct sockaddr *) &server_sa, 
        sizeof server_sa) < 0)
    {
        perror("create_tcp_client: connect");
        return -1;
    }

    /* it blocks on write */
    return server_fd;
}
