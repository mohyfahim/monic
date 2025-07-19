#pragma once

#ifdef __cplusplus
extern "C" {
#endif

char *dns_lookup(char *addr_host, struct sockaddr_in *addr_con);
char *reverse_dns_lookup(char *ip_addr);

#ifdef __cplusplus
}
#endif
