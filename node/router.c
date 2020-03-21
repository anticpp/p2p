#include "router.h"

enum p2p_router_status_s {
    P2P_ROUTER_STATUS_CONNECTING = 0;
    P2P_ROUTER_STATUS_HANDSHAKE;
    P2P_ROUTER_STATUS_OPEN;
};

typedef struct {
    p2p_nodeid_t dst;
    p2p_router_status_s status;
}p2p_router_internal_t;

p2p_router_t* p2p_router_create(p2p_node_id_t dst) {
    p2p_router_internal_t *r = malloc(sizeof(p2p_router_internal_t));
    if( !r ) {
        return NULL;
    }

    memcpy(r->dst, dst, P2P_ROUTER_SIZE);
    return (p2p_router_t *)r;
}

int p2p_router_destroy(p2p_router_t *r) {
    free(r);
    return 0;
}

int p2p_router_open(p2p_router_t *r, p2p_router_onopen_cb callback) {
    // Find dst address
    char dst_ip[32] = "10.10.4.6";
    short dst_port = 20013;
}


