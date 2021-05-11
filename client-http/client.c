#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

struct conninfos
{
    char ip[INET_ADDRSTRLEN];
    char port[6];
    in_port_t rawport;
};

static FILE *tcp_connect(const char *hostname, const char *port);

int main(int argc, char **argv)
{
    int result = EXIT_FAILURE;

    if (argc >= 3)
    {
        FILE *f = tcp_connect(argv[1], argv[2]);

        if (f != NULL)
        {
            char buffer[1024];

            fputs("GET / HTTP/1.1\r\nHost: gandalf.teleinf.labinfo.eiaj.ch\r\n\r\n", f);
            while (1)
            {
                char *res = fgets(buffer, sizeof(buffer), f);
                if (res != NULL)
                {
                    printf("%s", buffer); // puts(buffer);
                }
                else
                {
                    break;
                }
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

struct conninfos *getconninfos(struct addrinfo *rp)
{
    char ipname[INET_ADDRSTRLEN];
    char servicename[6]; // "65535\0"

    if (!getnameinfo(rp->ai_addr,
                     rp->ai_addrlen,
                     ipname,
                     sizeof(ipname),
                     servicename,
                     sizeof(servicename),
                     NI_NUMERICHOST | NI_NUMERICSERV))
    {

        struct conninfos *infos = malloc(sizeof(struct conninfos));
        infos->rawport = -1;

        if (rp->ai_family == AF_INET)
        {
            infos->rawport = ((struct sockaddr_in *)rp->ai_addr)->sin_port;
        }

        strcpy(infos->port, servicename);
        strcpy(infos->ip, ipname);

        return infos;
    }

    return NULL;
}

static FILE *tcp_connect(const char *hostname, const char *port)
{
    FILE *f = NULL;

    struct addrinfo hints, *res, *rp;
    int s;

    hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    s = getaddrinfo(hostname, port, &hints, &res);

    if (s != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    // getaddrinfo() returns a list of address structures.
    // Try each address until we successfully connect(2).
    // If socket(2) (or connect(2)) fails, we (close the socket
    // and) try the next address.
    for (rp = res; rp != NULL; rp = rp->ai_next)
    {
        struct conninfos *infos = getconninfos(rp);
        printf("Trying to connect to %s:%s (raw port: %d)\n", infos->ip, infos->port, infos->rawport);

        s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (s == -1)
        {
            printf("Cannot connect to %s:%s\n", infos->ip, infos->port);
            continue;
        }

        if (connect(s, rp->ai_addr, rp->ai_addrlen) != -1)
        {
            printf("Connected to %s:%s\n", infos->ip, infos->port);
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
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }

    f = fdopen(s, "r+");

    return f;
}
