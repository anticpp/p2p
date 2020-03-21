#include "net.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int net_inet_pton(int af, const char *ip, int port, struct sockaddr *addr) {
    addr->sa_family = af;

    if( af==AF_INET ) {
        struct sockaddr_in *saddr = (struct sockaddr_in*)addr;
        saddr->sin_port = htons(port);
        if( inet_pton(AF_INET, ip, &saddr->sin_addr)<0 )
            return -1;
    } else { /* AF_INET6 */
        struct sockaddr_in6 *saddr = (struct sockaddr_in6*)addr;
        saddr->sin6_port = htons(port);
        if( inet_pton(AF_INET6, ip, &saddr->sin6_addr)<0 )
            return -1;
    }
    return 0;
}

int net_inet_ntop(const struct sockaddr *addr, char *dst, socklen_t dst_len) {
    if(addr->sa_family==AF_INET) {
        inet_ntop(AF_INET, &(((struct sockaddr_in*)addr)->sin_addr), dst, dst_len);
    } else { /* AF_INET6*/
        inet_ntop(AF_INET6, &(((struct sockaddr_in6*)addr)->sin6_addr), dst, dst_len);
    }
    return 0;
}

int net_bind_address(int af, int fd, const char *ip, int port) {
    struct sockaddr addr;
    socklen_t addr_len;
    if( net_inet_pton(af, ip, port, &addr)<0 )
        return -1;
    addr_len = net_address_len(&addr);

    if( bind(fd, &addr, addr_len)<0 )
        return -1;

    return 0;
}

int net_set_reuse_address(int fd, int reuse) {
    int optval = reuse==1?1:0;
    if( setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))<0 )
        return -1;

    return 0;
}

socklen_t net_address_len(const struct sockaddr *addr) {
    if( addr->sa_family==AF_INET ) {
        return sizeof(struct sockaddr_in);
    } else { /* AF_INET6 */
        return sizeof(struct sockaddr_in6);
    }   
}

int net_address_port(const struct sockaddr *addr) {
    if( addr->sa_family==AF_INET ) {
        return ntohs(((struct sockaddr_in*)addr)->sin_port);
    } else { /* AF_INET6 */
        return ntohs(((struct sockaddr_in6*)addr)->sin6_port);
    }
}
