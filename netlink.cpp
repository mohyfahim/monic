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
#include "netlink.h"

#define MONIC_WAN_INTERFACE "wwan0"
#define MONIC_WAN_PORT "/dev/cdc-wdm0"

typedef struct {
  int fd;
  struct msghdr msg;
  struct sockaddr_nl local;
  char buf[8192];
} nl_payload_t;

// little helper to parsing message using netlink macroses
void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len) {
  memset(tb, 0, sizeof(struct rtattr *) * (max + 1));

  while (RTA_OK(rta, len)) { // while not end of the message
    if (rta->rta_type <= max) {
      tb[rta->rta_type] = rta; // read attr
    }
    rta = RTA_NEXT(rta, len); // get next attr
  }
}

int monic_netlink_task(std::shared_ptr<monic::state_t> state_ptr,
                       std::atomic<bool> *shutdown_requested_ptr,
                       std::mutex *mtx_ptr) {

  // netlink
  nl_payload_t nl_data;
  struct iovec iov;

  log_info("monic netlink started");

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
        parse_rtattr(tb, IFLA_MAX, IFLA_RTA(ifi),
                     h->nlmsg_len); // get attributes
        if (tb[IFLA_IFNAME]) {      // validation
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
        parse_rtattr(tba, IFA_MAX, IFA_RTA(ifa), h->nlmsg_len);

        if (tba[IFA_LOCAL]) {
          inet_ntop(AF_INET, RTA_DATA(tba[IFA_LOCAL]), ifAddress,
                    sizeof(ifAddress)); // get IP addr
        }

        {
          std::lock_guard<std::mutex> lock(*mtx_ptr);
          SampleModel sm{};
          sm.interface = std::string(ifName);
          std::string interface_status = std::string();
          bool supported = true;
          switch (h->nlmsg_type) { // what is actually happenned?
          case RTM_DELADDR:
            log_info("Interface %s: address was removed\n", ifName);
            interface_status = "DELADDR";
            break;
          case RTM_DELLINK:
            log_info("Network interface %s was removed\n", ifName);
            interface_status = "DELLINK";
            break;
          case RTM_NEWLINK:
            log_info("New network interface %s, state: %s %s\n", ifName, ifUpp,
                     ifRunn);
            interface_status =
                "NEWLINK_" + std::string(ifName) + "_" + std::string(ifUpp);
            break;
          case RTM_NEWADDR:
            log_info("Interface %s: new address was assigned: %s\n", ifName,
                     ifAddress);
            interface_status = "NEWADDR";
            sm.ip = std::string(ifAddress);
            break;
          default:
            log_warn("Unsupported Event");
            supported = false;
            break;
          }
          if (supported) {
            sm.interface_status = interface_status;
            sm.synced = false;
            sm.created_at = monic::get_current_epoch();
            sm.connection_status = state_ptr->connect;
            state_ptr->storage->insert(sm);
          }
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