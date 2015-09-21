#ifndef NETLIB_H
#define NETLIB_H

#include "common.h"

int listen_to_udp(char *portnum);
int get_udp_packet(int sockfd, void *buf, int bufsiz,
                   struct sockaddr_storage *remote_addr);
void *get_in_addr(struct sockaddr_storage *sas);

#endif // NETLIB_H
