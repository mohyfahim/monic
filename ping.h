#pragma once

#include <netinet/in.h>
#include <netinet/ip_icmp.h>
// Define the Packet Constants
#define PING_PKT_S 64           // ping packet size
#define PING_SLEEP_RATE 1000000 // ping sleep rate (in microseconds)
#define RECV_TIMEOUT 1          // timeout for receiving packets (in seconds)

typedef struct {
  struct icmphdr hdr;
  char msg[PING_PKT_S - sizeof(struct icmphdr)];
} ping_t;

#ifdef __cplusplus
extern "C" {
#endif

int monic_ping_host(char *host);

#ifdef __cplusplus
}
#endif
