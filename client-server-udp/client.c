#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "shared.h"

int main(int argc, char **argv) {
  int result = EXIT_FAILURE;

  if (argc >= 3) {
    FILE *f = udp_connect(argv[1], argv[2]);

    if (f != NULL) {
      while (1) {
        char message[buffer_size] = "";
        fgets(message, sizeof(message), stdin);
        fputs(message, f);
        fflush(f);
      }

      fclose(f);
      f = NULL;

      result = EXIT_SUCCESS;
    }
  } else {
    fprintf(stderr, "%s remote-host port\n", argv[0]);
    fprintf(stderr, "%s: bad args.\n", argv[0]);
  }

  return result;
}

static FILE *udp_connect(const char *hostname, const char *port) {
  FILE *f = NULL;

  struct addrinfo hints, *res, *rp;
  int s;

  hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
  hints.ai_socktype = SOCK_DGRAM; // UDP
  hints.ai_flags = 0;
  hints.ai_protocol = 0;

  s = getaddrinfo(hostname, port, &hints, &res);

  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  }

  // getaddrinfo() returns a list of address structures.
  // Try each address until we successfully connect(2).
  // If socket(2) (or connect(2)) fails, we (close the socket
  // and) try the next address.
  for (rp = res; rp != NULL; rp = rp->ai_next) {
    struct conninfos *infos = getconninfos(rp);
    printf("Trying to connect to %s:%s (raw port: %d)\n", infos->ip,
           infos->port, infos->rawport);

    s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

    if (s == -1) {
      printf("Cannot connect to %s:%s\n", infos->ip, infos->port);
      continue;
    }

    if (connect(s, rp->ai_addr, rp->ai_addrlen) != -1) {
      printf("Connected to %s:%s\n", infos->ip, infos->port);
      free(infos);
      break;
    }

    free(infos);
    close(s);
  }

  // No longer needed
  freeaddrinfo(res);

  if (rp == NULL) {
    // No address succeeded
    fprintf(stderr, "Could not connect\n");
    exit(EXIT_FAILURE);
  }

  f = fdopen(s, "r+");

  return f;
}
