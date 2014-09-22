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
  char* server = (argc >= 2 ? argv[1] : "127.0.0.1");
  int port = (argc >= 3 ? atoi(argv[2]) : 6500);
  struct sockaddr_in sockaddr;
  int listener_sd, rc;
  pid_t pid;

  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  inet_aton(server, &sockaddr.sin_addr);
  // sockaddr.sin_addr.s_addr = INADDR_LOOPBACK;
  sockaddr.sin_port = htons(port);

  int sd;
  int i = 0;
  char byte;
  sd = socket(AF_INET, SOCK_STREAM, 0); /* create connection socket */
  connect(sd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
  while (1) { /* client writes and then reads */
    if (i++ % 1000 == 0) {printf("."); fflush(stdout);}
    if (i % 50000 == 0) printf("\n");
    while (write(sd, &byte, 1) != 1) ;
    while (read(sd, &byte, 1) != 1);
  }

}
