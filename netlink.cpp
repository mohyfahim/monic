#include <arpa/inet.h>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"

#define MONIC_WAN_INTERFACE "wwan0"
#define MONIC_WAN_PORT "/dev/cdc-wdm0"

// static char *current_ip_address = NULL;

// int setup_netlink_socket() {
//   int sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
//   if (sock_fd < 0) {
//     log_message(LOG_ERR, "Failed to create Netlink socket: %s",
//                 strerror(errno));
//     return -1;
//   }

//   struct sockaddr_nl sa;
//   memset(&sa, 0, sizeof(sa));
//   sa.nl_family = AF_NETLINK;
//   // Listen for link state changes (RTMGRP_LINK) and IP address changes
//   // (RTMGRP_IPV4_IFADDR, RTMGRP_IPV6_IFADDR)
//   sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;

//   if (bind(sock_fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
//     log_message(LOG_ERR, "Failed to bind Netlink socket: %s",
//     strerror(errno)); close(sock_fd); return -1;
//   }

//   log_message(
//       LOG_INFO,
//       "Netlink socket successfully set up for monitoring network events.");
//   return sock_fd;
// }

// // --- Handle Netlink Events ---
// // Reads and parses Netlink messages to detect interface and IP changes.
// void handle_netlink_event(int sock_fd) {
//   char buffer[4096]; // Buffer for Netlink messages
//   struct iovec iov = {buffer, sizeof(buffer)};
//   struct sockaddr_nl sa;
//   struct msghdr msg = {(void *)&sa, sizeof(sa), &iov, 1, NULL, 0, 0};

//   ssize_t len = recvmsg(sock_fd, &msg, 0);
//   if (len < 0) {
//     if (errno == EINTR)
//       return; // Interrupted by signal, just return
//     log_message(LOG_ERR, "Netlink recvmsg error: %s", strerror(errno));
//     return;
//   }

//   struct nlmsghdr *nh;
//   // Iterate through Netlink messages in the buffer
//   for (nh = (struct nlmsghdr *)buffer; NLMSG_OK(nh, len);
//        nh = NLMSG_NEXT(nh, len)) {
//     if (nh->nlmsg_type == NLMSG_DONE)
//       break; // End of messages
//     if (nh->nlmsg_type == NLMSG_ERROR) {
//       log_message(LOG_ERR, "Netlink message error received.");
//       continue;
//     }

//     // Handle link state changes (interface up/down)
//     if (nh->nlmsg_type == RTM_NEWLINK || nh->nlmsg_type == RTM_DELLINK) {
//       struct ifinfomsg *ifinfo = (struct ifinfomsg *)NLMSG_DATA(nh);
//       char ifname[IF_NAMESIZE];
//       if_indextoname(ifinfo->ifi_index,
//                      ifname); // Get interface name from index

//       // Only process events for our target WWAN interface
//       if (strcmp(ifname, MONIC_WAN_INTERFACE) == 0) {
//         if (nh->nlmsg_type == RTM_NEWLINK) {
//           if (ifinfo->ifi_flags & IFF_UP) {
//             log_message(LOG_INFO,
//                         "Netlink Event: Network interface %s is now UP.",
//                         ifname);
//           } else {
//             log_message(LOG_INFO,
//                         "Netlink Event: Network interface %s is now DOWN.",
//                         ifname);
//             g_free(current_ip_address); // Clear stored IP on link down
//             current_ip_address = NULL;
//           }
//         } else { // RTM_DELLINK
//           log_message(LOG_INFO,
//                       "Netlink Event: Network interface %s is DELETED.",
//                       ifname);
//           g_free(current_ip_address);
//           current_ip_address = NULL;
//         }
//       }
//     }
//     // Handle IP address changes (new IP assigned, IP removed)
//     else if (nh->nlmsg_type == RTM_NEWADDR || nh->nlmsg_type == RTM_DELADDR)
//     {
//       struct ifaddrmsg *ifaddr = (struct ifaddrmsg *)NLMSG_DATA(nh);
//       char ifname[IF_NAMESIZE];
//       if_indextoname(ifaddr->ifa_index, ifname);

//       // Only process events for our target WWAN interface
//       if (strcmp(ifname, MONIC_WAN_INTERFACE) == 0) {
//         struct rtattr *rta;
//         char ip_str[INET6_ADDRSTRLEN]; // Buffer for IP address string

//         // Iterate through attributes to find the address
//         for (rta = IFA_RTA(ifaddr); RTA_OK(rta, IFA_PAYLOAD(ifaddr));
//              rta = RTA_NEXT(rta, IFA_PAYLOAD(ifaddr))) {
//           if (rta->rta_type == IFA_ADDRESS) {
//             // Convert binary IP to string
//             if (ifaddr->ifa_family == AF_INET) {
//               inet_ntop(AF_INET, RTA_DATA(rta), ip_str, sizeof(ip_str));
//             } else if (ifaddr->ifa_family == AF_INET6) {
//               inet_ntop(AF_INET6, RTA_DATA(rta), ip_str, sizeof(ip_str));
//             } else {
//               continue; // Skip unsupported address families
//             }

//             if (nh->nlmsg_type == RTM_NEWADDR) {
//               // Check if IP has actually changed or is new
//               if (current_ip_address == NULL ||
//                   strcmp(current_ip_address, ip_str) != 0) {
//                 log_message(LOG_INFO,
//                             "Netlink Event: IP address for %s changed to %s",
//                             ifname, ip_str);
//                 g_free(current_ip_address);            // Free old IP string
//                 current_ip_address = g_strdup(ip_str); // Store new IP
//               }
//             } else { // RTM_DELADDR
//               log_message(LOG_INFO,
//                           "Netlink Event: IP address %s removed from %s",
//                           ip_str, ifname);
//               if (current_ip_address &&
//                   strcmp(current_ip_address, ip_str) == 0) {
//                 g_free(current_ip_address);
//                 current_ip_address = NULL;
//               }
//             }
//           }
//         }
//       }
//     }
//   }
// }

// void monic_netlink_setup() {
//   // --- Setup Netlink socket for network event monitoring ---
//   int netlink_sock_fd = setup_netlink_socket();
//   if (netlink_sock_fd < 0) {
//     log_error("Failed to setup Netlink socket. Exiting monitor.");
//     return EXIT_FAILURE;
//   }
// }

typedef struct {
  int fd;
  struct msghdr msg;
  struct sockaddr_nl local;
  char buf[8192];
} nl_payload_t;

// little helper to parsing message using netlink macroses
void parseRtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len) {
  memset(tb, 0, sizeof(struct rtattr *) * (max + 1));

  while (RTA_OK(rta, len)) { // while not end of the message
    if (rta->rta_type <= max) {
      tb[rta->rta_type] = rta; // read attr
    }
    rta = RTA_NEXT(rta, len); // get next attr
  }
}

int monic_netlink_task(std::atomic<bool> *shutdown_requested_ptr) {

  // netlink
  nl_payload_t nl_data;
  struct iovec iov;

  log_info("nw_handler_task started");

  iov.iov_base = nl_data.buf;        // set message buffer as io
  iov.iov_len = sizeof(nl_data.buf); // set size
  nl_data.fd =
      socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE); // create netlink socket
  if (nl_data.fd < 0) {
    log_error("Failed to create netlink socket: %s", std::strerror(errno));
    return -1;
  }
  memset(&nl_data.local, 0, sizeof(nl_data.local));

  nl_data.local.nl_family = AF_NETLINK;
  nl_data.local.nl_groups =
      RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE;
  nl_data.local.nl_pid = getpid();
  {
    nl_data.msg.msg_name = &nl_data.local;
    nl_data.msg.msg_namelen = sizeof(nl_data.local);
    nl_data.msg.msg_iov = &iov;
    nl_data.msg.msg_iovlen = 1;
  }

  if (bind(nl_data.fd, (struct sockaddr *)&nl_data.local,
           sizeof(nl_data.local)) < 0) {
    log_error("Failed to bind netlink socket: %s", (char *)strerror(errno));
    close(nl_data.fd);
    return -1;
  }

  while (!(*shutdown_requested_ptr).load()) {

    ssize_t status = recvmsg(nl_data.fd, &(nl_data.msg), MSG_DONTWAIT);

    //  check status
    if (status < 0) {
      if (errno == EINTR || errno == EAGAIN) {
        log_debug("received EINTR or EAGAIN: %d ,  %s", errno,
                  (char *)strerror(errno));
        usleep(250000);
        continue;
      }
      log_error("Failed to read netlink: %s", (char *)strerror(errno));
      continue;
    }

    if (nl_data.msg.msg_namelen !=
        sizeof(nl_data.local)) { // check message length, just in case
      log_error("Invalid length of the sender address struct\n");
      continue;
    }

    // message parser
    struct nlmsghdr *h;

    for (h = (struct nlmsghdr *)nl_data.buf;
         status >= (ssize_t)sizeof(*h);) { // read all messagess headers
      int len = h->nlmsg_len;
      int l = len - sizeof(*h);
      char *ifName;

      if ((l < 0) || (len > status)) {
        log_error("Invalid message length: %i\n", len);
        continue;
      }

      // now we can check message type
      if ((h->nlmsg_type == RTM_NEWROUTE) ||
          (h->nlmsg_type == RTM_DELROUTE)) { // some changes in routing table
        log_info("Routing table was changed\n");
      } else { // in other case we need to go deeper
        char *ifUpp;
        char *ifRunn;
        struct ifinfomsg *ifi; // structure for network interface info
        struct rtattr *tb[IFLA_MAX + 1];

        ifi = (struct ifinfomsg *)NLMSG_DATA(
            h); // get information about changed network interface
        parseRtattr(tb, IFLA_MAX, IFLA_RTA(ifi),
                    h->nlmsg_len); // get attributes
        if (tb[IFLA_IFNAME]) {     // validation
          ifName =
              (char *)RTA_DATA(tb[IFLA_IFNAME]); // get network interface name
        }

        if (ifi->ifi_flags & IFF_UP) { // get UP flag of the network interface
          ifUpp = (char *)"UP";
        } else {
          ifUpp = (char *)"DOWN";
        }

        if (ifi->ifi_flags &
            IFF_RUNNING) { // get RUNNING flag of the network interface
          ifRunn = (char *)"RUNNING";
        } else {
          ifRunn = (char *)"NOT RUNNING";
        }

        char ifAddress[256];   // network addr
        struct ifaddrmsg *ifa; // structure for network interface data
        struct rtattr *tba[IFA_MAX + 1];

        ifa = (struct ifaddrmsg *)NLMSG_DATA(
            h); // get data from the network interface
        parseRtattr(tba, IFA_MAX, IFA_RTA(ifa), h->nlmsg_len);

        if (tba[IFA_LOCAL]) {
          inet_ntop(AF_INET, RTA_DATA(tba[IFA_LOCAL]), ifAddress,
                    sizeof(ifAddress)); // get IP addr
        }

        switch (h->nlmsg_type) { // what is actually happenned?
        case RTM_DELADDR:
          log_info("Interface %s: address was removed\n", ifName);
          break;

        case RTM_DELLINK:
          log_info("Network interface %s was removed\n", ifName);
          break;

        case RTM_NEWLINK:
          log_info("New network interface %s, state: %s %s\n", ifName, ifUpp,
                   ifRunn);
          break;

        case RTM_NEWADDR:
          log_info("Interface %s: new address was assigned: %s\n", ifName,
                   ifAddress);
          break;
        }
      }

      status -= NLMSG_ALIGN(
          len); // align offsets by the message length, this is important
      h = (struct nlmsghdr *)((char *)h + NLMSG_ALIGN(len)); // get next message
    }

    usleep(250000); // sleep for a while
  }
  close(nl_data.fd); // close socket

  return 0;
}