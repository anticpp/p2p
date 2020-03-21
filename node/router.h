#ifndef P2P_ROUTER_H
#define P2P_ROUTER_H

typedef struct {
    char data[1];
}p2p_router_t;

typedef void (*p2p_router_onopen_cb)(p2p_router_t *r, int err);

p2p_router_t* p2p_router_create(p2p_nodeid_t dst);

int p2p_router_destroy(p2p_router_t *r);

int p2p_router_open(p2p_router_t *r, p2p_router_onopen_cb callback);

#endif
