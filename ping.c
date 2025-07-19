#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "dns.h"
#include "ping.h"

// Calculate the checksum (RFC 1071)
unsigned short checksum(void *b, int len) {
  unsigned short *buf = b;
  unsigned int sum = 0;
  unsigned short result;

  for (sum = 0; len > 1; len -= 2)
    sum += *buf++;
  if (len == 1)
    sum += *(unsigned char *)buf;
  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  result = ~sum;
  return result;
}

// Make a ping request
void send_ping(int ping_sockfd, struct sockaddr_in *ping_addr, char *ping_dom,
               char *ping_ip, char *rev_host) {
  int ttl_val = 64, msg_count = 0, i, addr_len, flag = 1,
      msg_received_count = 0;
  char rbuffer[128];
  ping_t pckt;
  struct sockaddr_in r_addr;
  struct timespec time_start, time_end, tfs, tfe;
  long double rtt_msec = 0, total_msec = 0;
  struct timeval tv_out;
  tv_out.tv_sec = RECV_TIMEOUT;
  tv_out.tv_usec = 0;

  clock_gettime(CLOCK_MONOTONIC, &tfs);

  // Set socket options at IP to TTL and value to 64
  if (setsockopt(ping_sockfd, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) != 0) {
    printf("\nSetting socket options to TTL failed!\n");
    return;
  } else {
    printf("\nSocket set to TTL...\n");
  }

  // Setting timeout of receive setting
  setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv_out,
             sizeof tv_out);

  clock_gettime(CLOCK_MONOTONIC, &tfe);

  while (tfe.tv_sec - tfs.tv_sec < 3) {
    // Flag to check if packet was sent or not
    flag = 1;

    // Fill the packet
    bzero(&pckt, sizeof(pckt));
    pckt.hdr.type = ICMP_ECHO;
    pckt.hdr.un.echo.id = getpid();

    for (i = 0; i < sizeof(pckt.msg) - 1; i++)
      pckt.msg[i] = i + '0';

    pckt.msg[i] = 0;
    pckt.hdr.un.echo.sequence = msg_count++;
    pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));

    usleep(PING_SLEEP_RATE);

    // Send packet
    clock_gettime(CLOCK_MONOTONIC, &time_start);
    if (sendto(ping_sockfd, &pckt, sizeof(pckt), 0,
               (struct sockaddr *)ping_addr, sizeof(*ping_addr)) <= 0) {
      printf("\nPacket Sending Failed!\n");
      flag = 0;
    }

    memset(rbuffer, 0, sizeof rbuffer);

    // Receive packet
    addr_len = sizeof(r_addr);
    if (recvfrom(ping_sockfd, rbuffer, sizeof(rbuffer), 0,
                 (struct sockaddr *)&r_addr, &addr_len) <= 0 &&
        msg_count > 1) {
      printf("\nPacket receive failed!\n");
    } else {
      clock_gettime(CLOCK_MONOTONIC, &time_end);

      double timeElapsed =
          ((double)(time_end.tv_nsec - time_start.tv_nsec)) / 1000000.0;
      rtt_msec = (time_end.tv_sec - time_start.tv_sec) * 1000.0 + timeElapsed;

      // If packet was not sent, don't receive
      if (flag) {
        struct icmphdr *recv_hdr = (struct icmphdr *)rbuffer;
        if (!(recv_hdr->type == 69 && recv_hdr->code == 0)) {
          printf(
              "Error... Packet received with ICMP type %d code %d errorno %s\n",
              recv_hdr->type, recv_hdr->code, strerror(errno));
        } else {
          printf("%d bytes from %s (h: %s) (ip: %s) msg_seq = %d ttl = %d rtt "
                 "= %Lf ms.\n",
                 PING_PKT_S, ping_dom, rev_host, ping_ip, msg_count, ttl_val,
                 rtt_msec);
          msg_received_count++;
        }
      }
    }
    clock_gettime(CLOCK_MONOTONIC, &tfe);
  }
  double timeElapsed = ((double)(tfe.tv_nsec - tfs.tv_nsec)) / 1000000.0;
  total_msec = (tfe.tv_sec - tfs.tv_sec) * 1000.0 + timeElapsed;

  printf("\n=== %s ping statistics ===\n", ping_ip);
  printf("%d packets sent, %d packets received, %f%% packet loss. Total time: "
         "%Lf ms.\n\n",
         msg_count, msg_received_count,
         ((msg_count - msg_received_count) / (double)msg_count) * 100.0,
         total_msec);
}

// Driver Code
int monic_ping_host(char *host) {
  int sockfd;
  char *ip_addr, *reverse_hostname;
  struct sockaddr_in addr_con;
  int addrlen = sizeof(addr_con);
  char net_buf[NI_MAXHOST];

  ip_addr = dns_lookup(host, &addr_con);
  if (ip_addr == NULL) {
    printf("\nDNS lookup failed! Could not resolve hostname!\n");
    return 0;
  }

  reverse_hostname = reverse_dns_lookup(ip_addr);
  printf("\nTrying to connect to %s IP: %s\n", host, ip_addr);
  printf("\nReverse Lookup domain: %s\n", reverse_hostname);

  // Create a raw socket
  sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sockfd < 0) {
    printf("\nSocket file descriptor not received!: %s\n", strerror(errno));
    free(reverse_hostname);
    free(ip_addr);
    return 0;
  } else {
    printf("\nSocket file descriptor %d received\n", sockfd);
  }

  // Send pings continuously
  send_ping(sockfd, &addr_con, reverse_hostname, ip_addr, host);

  free(reverse_hostname);
  free(ip_addr);

  return 0;
}