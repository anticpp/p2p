#ifndef __NET_H__
#define __NET_H__
#include <arpa/inet.h>

/* Only for AF_INET and AF_INET6 */
int net_inet_pton(int af, const char *ip, int port, struct sockaddr *addr);
int net_inet_ntop(const struct sockaddr *addr, char *dst, socklen_t dst_len);
int net_bind_address(int af, int fd, const char *ip, int port);
int net_set_reuse_address(int fd, int reuse);
socklen_t net_address_len(const struct sockaddr *addr);
int net_address_port(const struct sockaddr *addr);

#endif
