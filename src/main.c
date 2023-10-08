/*
Simple chat server.

Douglas McCulloch, October 2023
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "server.h"
#include "client.h"

void show_help();

int main(int argc, char **argv)
{
    int f_server = 0, f_client = 0;
    int opt;

    /* [bin] [addr] [port1] [port2] -sc */
    /*  0       2       3     4     1  */
    while((opt = getopt(argc, argv, "sc")) != -1)
    {
        switch(opt)
        {
        case 's':
            f_server = 1;
            break;
        
        case 'c':
            f_client = 1;
            break;

        default:
            fputs("Switch not recognised.\n", stderr);
            exit(EXIT_FAILURE);
        }
    }

    if(argv[1] == NULL || argv[2] == NULL)
    {
        show_help();
        exit(EXIT_FAILURE);
    }

    if(f_server && f_client)
    {
        fputs("Can't be both server and client!\n"
            "Defaulting to client mode.\n", stderr);
    }

    short port1 = atoi(argv[3]);
    short port2 = atoi(argv[4]);

    if(f_client) return client(argv[2], port1, port2);
    else if(f_server) return server(port1, port2);

}

void show_help()
{
    fputs("Usage:\n"
        "[bin] [addr] [port] -sc\n", stderr);
}
