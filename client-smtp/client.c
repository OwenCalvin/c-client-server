#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SEPARATOR "\r\n"
#define MAX_FIELD_LENGTH 256

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

  // Greylisting
  if (strcmp(code, "450") == 0) {
    printf("450: You are greylisted, ");
    return 2;
  }

  switch (error_subject) {
  case '2':
    // printf("Success... (%s)\r\n", code);
  case '3':
    // printf("Waiting for more informations... (%s)\r\n", code);
    return 0;

  case '5':
    printf("5xx Permanent failure: %s\r\n", code);
    break;

  case '4':
    printf("4xx Temporary failure: %s\r\n", code);
    break;

  default:
    printf("Unknow error: %s\r\n", code);
    break;
  }

  return 1;
}

int main(int argc, char **argv) {
  int result = EXIT_FAILURE;
  int error = 0;

  char *arg_exp = argv[1];
  char *arg_subject = argv[2];
  char *arg_path = argv[3];
  char *arg_hostname = argv[4];
  char *arg_to = argv[5];
  char *arg_port = "25";

  if (argv[6] != 0) {
    arg_port = argv[6];
  }

  if (argc >= 6) {
    FILE *f = tcp_connect(arg_hostname, arg_port);

    if (f != NULL) {
      printf("Sending email...\n");

      char buffer[1024] = "";

      char helo[MAX_FIELD_LENGTH] = "HELO me";
      char mail_from[MAX_FIELD_LENGTH] = "";
      char rcpt_to[MAX_FIELD_LENGTH] = "";
      char data[MAX_FIELD_LENGTH] = "DATA";
      char from[MAX_FIELD_LENGTH] = "";
      char subject[MAX_FIELD_LENGTH] = "";
      char to[MAX_FIELD_LENGTH] = "";
      char *content = read_file(arg_path);
      char dot[MAX_FIELD_LENGTH] = ".";
      char quit[MAX_FIELD_LENGTH] = "QUIT";

      sprintf(helo, "%s" SEPARATOR, helo);
      sprintf(mail_from, "MAIL FROM: %s" SEPARATOR, arg_exp);
      sprintf(rcpt_to, "RCPT TO: %s" SEPARATOR, arg_to);
      sprintf(data, "%s" SEPARATOR, data);
      sprintf(subject, "Subject: %s" SEPARATOR, arg_subject);
      sprintf(from, "From: <%s>" SEPARATOR, arg_exp);
      sprintf(to, "To: %s" SEPARATOR, arg_to);
      sprintf(content, "%s" SEPARATOR, content);
      sprintf(dot, "%s" SEPARATOR, dot);
      sprintf(quit, "%s" SEPARATOR, quit);

      fputs(helo, f);
      fputs(mail_from, f);
      fputs(rcpt_to, f);
      fputs(data, f);
      fputs(subject, f);
      fputs(from, f);
      fputs(to, f);
      fputs(content, f);
      fputs(dot, f);
      fputs(quit, f);

      while (fgets(buffer, sizeof(buffer), f)) {
        if (strlen(buffer) > 0) {
          buffer[strlen(buffer) - 1] = '\0';
        }

        error = verify_error_code(buffer);

        if (error > 0) {
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

      if (error > 0) {
        result = EXIT_FAILURE;

        if (error == 2) {
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
