#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "shared.h"

int process_id = -1;

int main(int argc, char **argv) {
  int result = EXIT_FAILURE;

  if (argc >= 2) {
    create_server(argv[1]);
  } else {
    fprintf(stderr, "%s remote-host port\n", argv[0]);
    fprintf(stderr, "%s: bad args.\n", argv[0]);
  }

  return result;
}

static void create_server(const char *str_port) {
  FILE *f = NULL;
  unsigned short port = (unsigned short)atoi(str_port);
  struct sockaddr_in client, server;
  int s, new_socket;
  char *hostname = "127.0.0.1";

  // Get a socket for accepting connections
  s = socket(AF_INET, SOCK_STREAM, 0);

  if (s < 0) {
    perror("Socket()");
    exit(0);
  }
  printf("Socket created on %s:%s\n", hostname, str_port);

  // Bind the socket to the server address
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_port = htons(port);

  // Binding newly created socket to given IP
  if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("Bind()");
    exit(1);
  }
  printf("Successfully binded\n");

  // Listen for connections (max 128)
  if (listen(s, 128) != 0) {
    perror("Listen()");
    exit(2);
  }
  printf("Listening...\n");

  while (1) {
    // Accept a connection
    int namelen = sizeof(client);
    if ((new_socket = accept(s, (struct sockaddr *)&client, &namelen)) == -1) {
      perror("Accept()");
      exit(3);
    }

    process_id++;
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client.sin_addr), ip, INET_ADDRSTRLEN);
    printf("Accepted: %s:%s (id: %d)\n", ip, str_port, process_id);

    pid_t childpid = fork();

    if (childpid == 0) {
      // It's a child process
      // We close the server socket on the child process
      close(s);

      communicate(fdopen(new_socket, "r+"));

      // After the communicate function end, close the new_socket socket
      close(new_socket);
      exit(0);
    }

    // The parent process do not operate on the socket, so we close it
    close(new_socket);
  }
}

void communicate(FILE *f) {
  if (f != NULL) {
    while (1) {
      char buffer[buffer_size];
      char *res = fgets(buffer, sizeof(buffer), f);
      printf("%d: %s", process_id, buffer);
    }

    fclose(f);
    f = NULL;
  }
}
