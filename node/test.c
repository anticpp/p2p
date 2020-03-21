#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h> 
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int p2p_bind_fd(int fd, const char ip[], short port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if( inet_pton(AF_INET, ip, &addr.sin_addr)<0 ) {
        return -1;
    }

    int optval = 1;
    if( setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval))<0 ) {
        return -1;
    }

    if( bind(fd, (struct sockaddr*)&addr, sizeof(addr))<0 ) {
        return -1;
    }
    return 0;
}

///////////////// p2p_server_t
typedef struct {
    int sfd;
    char bind_ip[32];
    short bind_port;

    int connect_fds[100];
    int connect_fds_idx;
}p2p_server_t;

p2p_server_t *p2p_server_create() {
    int sfd = socket(PF_INET, SOCK_STREAM, 0);
    if(sfd<0) {
        return NULL;
    }

    p2p_server_t *s = malloc(sizeof(p2p_server_t));
    s->sfd = sfd;
    s->connect_fds_idx = 0;
    return s;
}

void psp_server_destroy(p2p_server_t *s) {
    free(s);
}

int p2p_server_listen(p2p_server_t *s) {
    printf("listening on %s:%d...\n", s->bind_ip, s->bind_port);
    
    if( p2p_bind_fd(s->sfd, s->bind_ip, s->bind_port)<0 ) {
        return -1;
    }

    if( listen(s->sfd, 20000)<0 ) {
        printf("listen fail: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int p2p_server_connect(p2p_server_t *s, const char ip[], short port) {
    printf("connecting on %s:%d\n", ip, port);
    int cfd = socket(PF_INET, SOCK_STREAM, 0);
    if( cfd<0 ) {
        printf("socket error: %s\n", strerror(errno));
        return -1;
    }

    if( p2p_bind_fd(cfd, s->bind_ip, s->bind_port)<0 ) {
        printf("p2p_bind_fd fail\n");
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if( inet_pton(AF_INET, ip, &addr.sin_addr)<0 ) {
        printf("inet_pton fail\n");
        return -1;
    }

    if( connect(cfd, (struct sockaddr *)&addr, sizeof(addr))<0 ) {
        printf("connect remote address(%s:%d) fail: %s\n", ip, port, strerror(errno));
    } else {
        printf("connect remote address(%s:%d) success\n", ip, port);
        s->connect_fds[s->connect_fds_idx++] = cfd;
    }

    return 0;
}
///////////////////

void usage(const char *prog) {
    printf("usage: %s [-i bind_ip] [-p bind_port] [-l] [remote ip] [remote port]\n", prog);
    return;
}

int main(int argc, char *argv[])
{
    p2p_server_t *server = p2p_server_create();
    bool if_listen = false;

    int ch;
    for(;;) {
        ch = getopt(argc, argv, "i:p:lh");
        if(ch==-1) {
            break;
        }

        switch(ch) {
            case 'i':
                memcpy(server->bind_ip, optarg, strlen(optarg));
                break;
            case 'p':
                server->bind_port = atoi(optarg);
                break;
            case 'l':
                if_listen = true;
                break;
            case 'h':
            default:
                usage(argv[0]);
                return 1;
        }
    }
    argc -= optind;
    argv += optind;

    printf("bind_ip: %s, bind_port: %d, listen: %d\n", server->bind_ip, server->bind_port, if_listen);

    // Listen
    if( if_listen ) {
        if( p2p_server_listen(server) ) {
            printf("listen error\n");
            return 1;
        }
    }
    
    // Remote
    char remote_ip[32] = {'\0'};
    short remote_port = 0;
    if( argc>0 ) {
        memcpy(remote_ip, argv[0], strlen(argv[0]));
    }
    if( argc>1 ) {
        remote_port = atoi(argv[1]);
    }
    printf("remote_ip: %s, remote_port: %d\n", remote_ip, remote_port);
    if( strlen(remote_ip)>0 ) {
        if( p2p_server_connect(server, remote_ip, remote_port)<0 ) {
            printf("connect fail\n");
            return 1;
        }
    }

    printf("tick");
    for(;;) {
        sleep(1);
        printf(".");
        fflush(stdout);
    }

    return 0;
}
