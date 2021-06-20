#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SEPARATOR "\r\n"

static FILE *tcp_connect(const char *hostname, const char *str_port);
int parse_error_code(char *buffer);
char *read_file(char *path);
void get_command(char *dest[2], int index, char **argv);
void send_command(FILE *f, int index, char **argv);

/**
 * Read the content of a file
 * @param path The file path
 */
char *read_file(char *path) {
  char *buffer = 0;
  long length;
  FILE *f = fopen(path, "rb");

  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(length);
    if (buffer) {
      fread(buffer, 1, length, f);
    }
    fclose(f);
  }

  return buffer;
}

/**
 * Parse the error code to get something that is easiest to treat in the main
 * program
 * @return An int that represent the error code (2xx 3xx 4xx 5xx 221 354 or 450)
 * @param buffer The response from the server
 */
int parse_error_code(char *buffer) {
  char code[4];
  strncpy(code, buffer, 3);
  code[3] = 0;

  char error_subject = code[0];

  printf("%s\r\n", buffer);

  switch (error_subject) {
  case '2':
    if (strcmp(code, "220") == 0)
      return atoi(code);

    if (strcmp(code, "221") == 0)
      return atoi(code);

    if (strcmp(code, "250") == 0)
      return atoi(code);

    return 2;

  case '3':
    if (strcmp(code, "354") == 0)
      return atoi(code);
    return 3;

  case '4':
    // Greylisting
    if (strcmp(code, "450") == 0)
      return atoi(code);
    printf("4xx Temporary failure: %s\r\n", code);
    return 4;

  case '5':
    printf("5xx Permanent failure: %s\r\n", code);
    return 5;

  default:
    printf("Unknow error: %s\r\n", code);
    return 6;
  }
}

/**
 * Get the command pair, with the format and the variable in the correct order
 * for the SMTP
 * @param dest The destination variable
 * @param index The index of the command
 * @param argv The argv of the program
 */
void get_command(char *dest[2], int index, char **argv) {
  char *arg_exp = argv[1];
  char *arg_subject = argv[2];
  char *arg_path = argv[3];
  char *arg_to = argv[5];

  char *helo = "HELO me";
  char *mail_from = "";
  char *rcpt_to = "";
  char *data = "DATA";
  char *from = "";
  char *subject = "";
  char *to = "";
  char *content = read_file(arg_path);
  char *dot = ".";
  char *quit = "QUIT";

  char *commands[][2] = {{"%s" SEPARATOR, helo},
                         {"MAIL FROM: %s" SEPARATOR, arg_exp},
                         {"RCPT TO: %s" SEPARATOR, arg_to},
                         {"%s" SEPARATOR, data},
                         {"Subject: %s" SEPARATOR, arg_subject},
                         {"From: <%s>" SEPARATOR, arg_exp},
                         {"To: %s" SEPARATOR, arg_to},
                         {"%s" SEPARATOR, content},
                         {"%s" SEPARATOR, dot},
                         {"%s" SEPARATOR, quit}};

  dest[0] = commands[index][0];
  dest[1] = commands[index][1];
}

void send_command(FILE *f, int index, char **argv) {
  char *command[2];
  get_command(command, index, argv);
  fprintf(stdout, command[0], command[1]);
  fprintf(f, command[0], command[1]);
}

int main(int argc, char **argv) {
  int result = EXIT_FAILURE;
  int error = 0;

  char *arg_hostname = argv[4];
  char *arg_port = "25";

  if (argv[6] != 0) {
    arg_port = argv[6];
  }

  if (argc >= 6) {
    FILE *f = tcp_connect(arg_hostname, arg_port);

    if (f != NULL) {
      printf("Sending email...\n");

      char buffer[1024] = "";

      // Send the first command HELO me
      int commandIndex = 0;
      send_command(f, commandIndex, argv);

      while (fgets(buffer, sizeof(buffer), f)) {
        if (strlen(buffer) > 0) {
          buffer[strlen(buffer) - 1] = '\0';
        }

        error = parse_error_code(buffer);
        int break_loop = 0;

        switch (error) {
        // Treat DATA (354) and send all the command until we get the command
        // "."
        case 354: {
          int is_not_dot = 1;
          while (is_not_dot) {
            commandIndex++;
            send_command(f, commandIndex, argv);

            char *command[2];
            get_command(command, commandIndex, argv);
            is_not_dot = strcmp(command[1], ".") != 0;
          }
          break;
        }

        // Treat normal case with error code 250 or 220
        case 220:
        case 250:
          commandIndex++;
          send_command(f, commandIndex, argv);
          break;

        // Break the loop for all these error code
        // could only use default but I wanted it to be explicit
        case 221:
        case 450:
        case 4:
        case 5:
        case 6:
        default:
          break_loop = 1;
          break;
        }

        if (break_loop) {
          break;
        }
      }

      shutdown(fileno(f), SHUT_WR);

      if (fclose(f) == 0) {
        f = NULL;
        result = EXIT_SUCCESS;
      } else {
        perror("fclose(): failed: ");
      }

      if (error > 3) {
        result = EXIT_FAILURE;

        // If we get a 450 error (Greylisted) we retry after 15 minutes
        // by recalling main with the same arguments
        // (and we return the result of the main recall)
        if (error == 450) {
          int minutes = 15;
          printf("we will retry in %d minutes...\r\n", minutes);
          sleep(60 * minutes);
          return main(argc, argv);
        }
      }
    }
  } else {
    fprintf(stderr, "%s remote-host port\n", argv[0]);
    fprintf(stderr, "%s: bad args.\n", argv[0]);
  }

  return result;
}

static FILE *tcp_connect(const char *hostname, const char *port) {
  FILE *f = NULL;

  int s;
  struct addrinfo hints;
  struct addrinfo *result, *rp;

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;

  if ((s = getaddrinfo(hostname, port, &hints, &result))) {
    fprintf(stderr, "getaddrinfo(): failed: %s.\n", gai_strerror(s));
  } else {
    for (rp = result; rp != NULL; rp = rp->ai_next) {
      char ipname[INET6_ADDRSTRLEN];
      char servicename[6];

      if ((s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
        perror("socket() failed: ");
        continue;
      }

      if (!getnameinfo(rp->ai_addr, rp->ai_addrlen, ipname, sizeof(ipname),
                       servicename, sizeof(servicename),
                       NI_NUMERICHOST | NI_NUMERICSERV)) {
        printf("Essai de connection vers host %s:%s ...\n", ipname,
               servicename);
      }

      if (connect(s, rp->ai_addr, rp->ai_addrlen) != -1) {
        break;
      } else {
        perror("connect()");
      }

      close(s);
    }

    freeaddrinfo(result);

    if (rp == NULL) {
      fprintf(stderr, "Could not connect.\n");
    } else {
      if (!(f = fdopen(s, "r+"))) {
        close(s);
        perror("fdopen() failed: ");
      }
    }
  }
  return f;
}
