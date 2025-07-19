#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#include "dns.h"

int monic_tcp_ip(char *ip, int port) {
  int sockfd, connfd;
  struct sockaddr_in servaddr;

  // socket create and verification
  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  if (sockfd == -1) {
    printf("socket creation failed: %s\n", strerror(errno));
    return -1;
  } else
    printf("Socket successfully created..\n");

  bzero(&servaddr, sizeof(servaddr));

  // assign IP, PORT
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(ip);
  servaddr.sin_port = htons(port);

  // connect the client socket to server socket
  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
    printf("connection with the server failed...\n");
    return -1;
  } else
    printf("connected to the server..\n");

  // close the socket
  close(sockfd);

  return 0;
}

int monic_tcp_host(char *host, int port) {
  int sockfd, connfd;
  struct sockaddr_in servaddr;

  bzero(&servaddr, sizeof(servaddr));

  char *ip_addr = dns_lookup(host, &servaddr);
  if (ip_addr == NULL) {
    printf("\nDNS lookup failed! Could not resolve hostname!\n");
    return -1;
  }
  servaddr.sin_port = htons(port);

  // socket create and verification
  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  if (sockfd == -1) {
    printf("socket creation failed: %s\n", strerror(errno));
    return -1;
  } else
    printf("Socket successfully created..\n");

  // connect the client socket to server socket
  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
    printf("connection with the server failed...\n");
    return -1;
  } else
    printf("connected to the server..\n");

  // close the socket
  close(sockfd);
  free(ip_addr);
  return 0;
}