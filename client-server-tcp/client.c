#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "shared.h"

int main(int argc, char **argv)
{
    int result = EXIT_FAILURE;

    if (argc >= 3)
    {
        FILE *f = tcp_connect(argv[1], argv[2]);

        if (f != NULL)
        {
            while (1)
            {
                char message[buffer_size] = "";
                fgets(message, sizeof(message), stdin);
                fputs(message, f);
                fflush(f);
            }

            fclose(f);
            f = NULL;

            result = EXIT_SUCCESS;
        }
    }
    else
    {
        fprintf(stderr, "%s remote-host port\n", argv[0]);
        fprintf(stderr, "%s: bad args.\n", argv[0]);
    }

    return result;
}

static FILE *tcp_connect(const char *hostname, const char *str_port)
{
    FILE *f = NULL;

    unsigned short port = (unsigned short)atoi(str_port);
    struct sockaddr_in client, server;
    int s, ns;

    // Get a socket for accepting connections
    s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0)
    {
        perror("Socket()");
        exit(0);
    }
    else
    {
        printf("Socket created\n");
    }

    // Bind the socket to the server address
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(hostname);
    server.sin_port = htons(port);

    if (connect(s, (struct sockaddr *)&server, sizeof(server)) != 0)
    {
        perror("Connect()");
        exit(1);
    }
    else
    {
        printf("Connected to %s:%s\n", hostname, str_port);
    }

    f = fdopen(s, "r+");

    return f;
}
