#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "shared.h"

int main(int argc, char **argv)
{
    int result = EXIT_FAILURE;

    if (argc >= 2)
    {
        FILE *f = create_server(argv[1]);

        if (f != NULL)
        {
            while (1)
            {
                char buffer[buffer_size];
                char *res = fgets(buffer, sizeof(buffer), f);
                printf("%s", buffer);
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

static FILE *create_server(const char *port)
{
    FILE *f = NULL;

    char buffer[1024];
    struct addrinfo hints, *res, *rp;
    int s;

    hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM; // UDP
    hints.ai_flags = AI_PASSIVE;    // For wildcard IP address
    hints.ai_protocol = 0;          // Any protocol
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    s = getaddrinfo(NULL, port, &hints, &res);

    if (s != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    // getaddrinfo() returns a list of address structures.
    // Try each address until we successfully bind(2).
    // If socket(2) (or bind(2)) fails, we (close the socket
    // and) try the next address.
    for (rp = res; rp != NULL; rp = rp->ai_next)
    {
        struct conninfos *infos = getconninfos(rp);
        printf("Trying to bind to %s:%s (raw port: %d)\n", infos->ip, infos->port, infos->rawport);

        s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (s == -1)
        {
            printf("Cannot bind to %s:%s\n", infos->ip, infos->port);
            continue;
        }

        if (bind(s, rp->ai_addr, rp->ai_addrlen) != -1)
        {
            printf("Binded to %s:%s\n", infos->ip, infos->port);
            free(infos);
            break;
        }

        free(infos);
        close(s);
    }

    // No longer needed
    freeaddrinfo(res);

    if (rp == NULL)
    {
        // No address succeeded
        fprintf(stderr, "Could not bind\n");
        exit(EXIT_FAILURE);
    }

    f = fdopen(s, "r+");

    return f;
}
