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

/*
- OK: e-mail expéditeur
- OK: sujet du message (entre guillemets si multi-mots, cf bash)
- OK: nom de fichier du corps du message
- OK: nom de domaine DNS ou adresse IP du serveur de mail à utiliser
- OK: e-mail destinataire
- OK: argument optionnel: numéro de port (par défaut 25)
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

int verify_error_code(char *buffer) {
  char code[4];
  strncpy(code, buffer, 3);
  code[3] = 0;

  char error_subject = code[0];

  printf("%s\r\n", buffer);

  switch (error_subject) {
  case '2':
    if (strcmp(code, "221") == 0)
      return atoi(code);
    // printf("Success... (%s)\r\n", code);
    return 2;

  case '3':
    // printf("Waiting for more informations... (%s)\r\n", code);
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

      int commandIndex = 0;
      send_command(f, commandIndex, argv);
      while (fgets(buffer, sizeof(buffer), f)) {
        if (strlen(buffer) > 0) {
          buffer[strlen(buffer) - 1] = '\0';
        }

        error = verify_error_code(buffer);
        int break_loop = 0;

        switch (error) {
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

        case 2:
          commandIndex++;
          send_command(f, commandIndex, argv);
          break;

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
