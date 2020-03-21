#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "connective.h"
#include "net.h"
#include "log.h"

#define DEFAULT_BIND_IP "127.0.0.1"
#define DEFAULT_BIND_PORT  20012

typedef struct {
    int sfd;
    char ip[32];
    int port;

    /* V4 address */
    struct sockaddr_in addr;
} node;

/* Global */
static node n = {0, DEFAULT_BIND_IP, DEFAULT_BIND_PORT};
connective *c = 0;

void read_input(int fd);
void usage(const char *prog) {
    fprintf(stderr, "usage: %s [options] [remote ip] [remote port]\n"\
                    "\n"\
                    "options:\n"
                    "  -i string\tbind ip, default '%s'\n"\
                    "  -p int\tbind port, default %d\n", 
                    prog, DEFAULT_BIND_IP, DEFAULT_BIND_PORT);
    return;
}

static int on_send_data(connective* c,
                    const struct sockaddr *local,
                    const struct sockaddr *remote, 
                    const char *data,
                    int data_len) {
    socklen_t remote_len = net_address_len(remote);
    return sendto(n.sfd, data, data_len, 0, remote, remote_len);
}

static int on_state_change (connective *c,
                    const struct sockaddr *local,
                    const struct sockaddr *remote,
                    connective_state l_state,
                    connective_state r_state) {
    char addr_local[100], addr_remote[100];
    int port_local, port_remote;
    net_inet_ntop(local, addr_local, sizeof(addr_local));
    net_inet_ntop(remote, addr_remote, sizeof(addr_remote));
    port_local = net_address_port(local);
    port_remote = net_address_port(remote);

    log_trace("on_state_change, Local{%s:%d}-->Remote{%s:%d}, l_state: %s, r_state: %s.\n",
                    addr_local, port_local,
                    addr_remote, port_remote,
                    connective_state_string[l_state],
                    connective_state_string[r_state]);
    return 0;
}
int on_timeout(connective *c,
                    const struct sockaddr *local,
                    const struct sockaddr *remote) {
    char addr_local[100], addr_remote[100];
    int port_local, port_remote;
    net_inet_ntop(local, addr_local, sizeof(addr_local));
    net_inet_ntop(remote, addr_remote, sizeof(addr_remote));
    port_local = net_address_port(local);
    port_remote = net_address_port(remote);

    log_trace("on_timeout, Local{%s:%d}-->Remote{%s:%d}.\n",
                    addr_local, port_local,
                    addr_remote, port_remote);
    return 0;
}

int main(int argc, char **argv) {
    const char *prog = argv[0];
    /* Remote address */
    char rip[32] = {'\0'};
    int rport = 0;
    struct sockaddr_in raddr;

    for(;;) {
        int ch = getopt(argc, argv, "i:p:lh");
        if(ch==-1) {
            break;
        }

        switch(ch) {
            case 'i':
                memcpy(n.ip, optarg, strlen(optarg));
                break;
            case 'p':
                n.port = atoi(optarg);
                break;
            case 'h':
            default:
                usage(prog);
                return 1;
        }
    }
    argc -= optind;
    argv += optind;

    if(argc<2) {
        usage(prog);
        return 1;
    }

    memcpy(rip, argv[0], strlen(argv[0]));
    rport = atoi(argv[1]);

    log_trace("Bind address %s:%d\n", n.ip, n.port);
    log_trace("Remote address %s:%d\n", rip, rport);
    
    /* To inet address */
    net_inet_pton(AF_INET, n.ip, n.port, (struct sockaddr *)&n.addr);
    net_inet_pton(AF_INET, rip, rport, (struct sockaddr *)&raddr);

    /* Bind socket */
    n.sfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(n.sfd<0) {
        log_error("Create socket error%s\n", strerror(errno));
        return 1;
    }
    net_set_reuse_address(n.sfd, 1);
    if( bind(n.sfd, (struct sockaddr *)&n.addr, sizeof(n.addr)) <0 ) {
        log_error("Bind address error%s\n", strerror(errno));
        return 1;
    }

    /* Create connective check */
    c = connective_create();
    connective_events cevents = {
        on_send_data,
        on_state_change,
        on_timeout
    };
    connective_attach_events(c, cevents);
    connective_create_check(c,
                (struct sockaddr*)&n.addr, 
                (struct sockaddr*)&raddr,
                0);

    /* Poll server fd */
    struct pollfd fds[10];
    memset(fds, 0x00, 10*sizeof(struct pollfd));
    fds[0].fd = n.sfd;
    fds[0].events = POLLIN;

    int now, last_p_time;
    int stop = 0;
    while(!stop) {
        now = time(0);
        int events, e0, e1 =0;

        /* Poll events */
        e0 = poll(fds, 1, 10);
        if ( e0<0 ) {
            log_error("poll error: %s\n", strerror(errno));
            continue;
        }
        events += e0;

        for(int i=0; i<e0; i++) {
            /* Listen fd */
            if( fds[i].fd==n.sfd ) {
                if( fds[i].revents&POLLERR ||
                                fds[i].revents&POLLHUP ||
                                fds[i].revents&POLLNVAL ) {
                    log_error("poll fail for fd:%d, revent: %d\n", n.sfd, fds[i].revents);
                    continue;
                }
                if( fds[i].revents&POLLIN ) {
                    read_input(fds[i].fd);
                }
            }
        }

        /* Connective drive */
        e1 = connective_drive(c);
        events += e1;

        /* Internal monitor */
        if( now - last_p_time>5 ) {
            last_p_time = now;
            connective_print_status(c);
        }
    
        /* Sleep for a while if not events happen */
        if( events==0 ) {
            usleep(10*1000);
        }
    }

    /* Clean up */
    connective_destroy(c);

    return 0;
}

void read_input(int fd) {
    struct sockaddr_in remote;
    socklen_t remote_len = sizeof(remote);
    char input[2048] = {0};

    ssize_t bytes = recvfrom(n.sfd, 
                    input, sizeof(input), 0, 
                    (struct sockaddr *)&remote, &remote_len);
    if(bytes<0) {
        log_error("recvfrom error: %s\n", strerror(errno));
        return ;
    }

    /* Drive connective */
    connective_on_recv_data(c, 
                    (struct sockaddr *)&n.addr, 
                    (struct sockaddr *)&remote, 
                    input, bytes);
}
