#define _BSD_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <stdio.h>

int main(int argc, char *argv[]) {
  int port = (argc == 2 ? atoi(argv[1]) : 6500);
  printf("Listening on port %d \n", port );
  struct sockaddr_in sockaddr;
  int listener_sd, rc;
  pid_t pid;

  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  inet_aton("192.168.200.2", &sockaddr.sin_addr);
  // sockaddr.sin_addr.s_addr = INADDR_LOOPBACK;
  sockaddr.sin_port = htons(port);

  listener_sd = socket(AF_INET, SOCK_STREAM, 0);
  do {
    sockaddr.sin_port = htons(ntohs(sockaddr.sin_port)+1);
    rc = bind(listener_sd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
  } while (rc == -1 && errno == EADDRINUSE);
  if (rc == -1)
    perror("bind");
  listen(listener_sd, 5);

  int sd;
  char byte;
  sd = accept(listener_sd, NULL, NULL);
  while (1) { /* server reads and then writes */
    while (read(sd, &byte, 1) != 1);
    while (write(sd, &byte, 1) != 1) ;
  }
}
