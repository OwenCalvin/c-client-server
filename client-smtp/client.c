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
- e-mail expéditeur,
- sujet du message (entre guillemets si multi-mots, cf bash)
- nom de fichier du corps du message
- nom de domaine DNS ou adresse IP du serveur de mail à utiliser
- e-mail destinataire
- argument optionnel: numéro de port (par défaut 25)
*/

int main(int argc, char **argv) {
  int result = EXIT_FAILURE;

  char *arg_exp = argv[1];
  char *arg_subject = argv[2];
  char *arg_message = argv[3];
  char *arg_hostname = argv[4];
  char *arg_to = argv[5];
  char *arg_port = argv[6];

  if (argc >= 0) {
    FILE *f = tcp_connect(arg_hostname, arg_port);

    if (f != NULL) {
      char buffer[1024] = "";

      char helo[256] = "HELO me";
      char mail_from[256] = "";
      char rcpt_to[256] = "";
      char data[256] = "DATA";
      char from[256] = "";
      char subject[256] = "";
      char to[256] = "";
      char content[256] = "hello world";
      char dot[256] = ".";
      char quit[256] = "QUIT";

      printf("Sending email...\n");

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
        puts(buffer);
      }

      shutdown(fileno(f), SHUT_WR);

      if (fclose(f) == 0) {
        f = NULL;

        result = EXIT_SUCCESS;
      } else {
        perror("fclose(): failed: ");
      }
    }
  } else {
    fprintf(stderr, "%s remote-host port\n", argv[0]);
    fprintf(stderr, "%s: bad args.\n", argv[0]);
  }

  return result;
}

static FILE *tcp_connect(const char *hostname, const char *str_port) {
  FILE *f = NULL;

  unsigned short port = (unsigned short)atoi(str_port);
  struct sockaddr_in client, server;
  int s, ns;

  // Get a socket for accepting connections
  s = socket(AF_INET, SOCK_STREAM, 0);

  if (s < 0) {
    perror("Socket()");
    exit(0);
  } else {
    printf("Socket created\n");
  }

  // Bind the socket to the server address
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(hostname);
  server.sin_port = htons(port);

  if (connect(s, (struct sockaddr *)&server, sizeof(server)) != 0) {
    perror("Connect()");
    exit(1);
  } else {
    printf("Connected to %s:%s\n", hostname, str_port);
  }

  f = fdopen(s, "r+");

  return f;
}
