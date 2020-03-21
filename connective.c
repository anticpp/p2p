#include "connective.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "net.h"
#include "log.h"
#include "array.h"

/*
 * Connective packet
 * --------------------------------
 * |pack(1) |version(1)|reserve(2)|
 * --------------------------------
 */

static const int request_internal_ = 2;
static const int request_timeout_ = 10;
static const int ping_internal_ = 5;

typedef enum {
    p_req = 0,
    p_resp,
    p_ping_req,
    p_ping_resp
} pack_;

const static char* pack_type_string[] = {
    "p_req",
    "p_resp",
    "p_ping_req",
    "p_ping_resp"
};

typedef struct {
    struct sockaddr local, remote;

    /* s_init      ---> (send request)
     *  s_req_sent ---> (recv response)
     *   s_done
     */
    connective_state l_state; 

    /* s_init        --->        (recv request)
     *  s_req_recvd  --->        (send response)
     *   s_done
     */
    connective_state r_state;

    /* As TCP's last ack.
     * When r_state enter `s_done`, a response sent to the other side.
     * However the response may lost on air, 
     * the other side must do an internal retry until a response received or timeout-fail,
     * this side should always send response on this situation.
     *
     * Set this signal on to indicate a response to send.
     */
    int insist_resp;

    int last_req_time;
    int first_req_time;
    int last_ping_time;

    /* Indicate state changed from last notification.
     */ 
    int state_changed;

    int timeout;
} pair_;

typedef struct {
    /*pair_ *pairs;
    int pairs_used;
    int pairs_size;*/
    array *pairs;

    connective_events events;
} connective_;

/* Private functions */
static pair_* connective_lookup_(connective *c,
                const struct sockaddr* local,
                const struct sockaddr* remote) {
    connective_ *cc = (connective_*)c;
    socklen_t addr_len1, addr_len2;
    pair_ *found = 0;
    for(int i=0; i<cc->pairs->size; i++) {
        pair_* pr = array_at(cc->pairs, i);
        if( local->sa_family!=pr->local.sa_family ||
                        remote->sa_family!=pr->remote.sa_family )
            continue;

        addr_len1 = net_address_len(local);
        addr_len2 = net_address_len(remote);
        if( memcmp(local, &pr->local, addr_len1)!=0 
                        || memcmp(remote, &pr->remote, addr_len2)!=0 )
            continue;

        found = pr;
        break;
    }
    return found;
}

static pair_* connective_add_pair_(connective *c,
                const struct sockaddr *local,
                const struct sockaddr *remote) {
    pair_ pr;

    socklen_t addr_len1 = 0, addr_len2 = 0;
    addr_len1 = net_address_len(local);
    addr_len2 = net_address_len(remote);

    memcpy(&pr.local, local, addr_len1);
    memcpy(&pr.remote, remote, addr_len2);
    pr.l_state = s_init;
    pr.r_state = s_init;
    pr.insist_resp = 0;
    pr.last_req_time = 0;
    pr.first_req_time = 0;
    pr.last_ping_time = 0;
    pr.state_changed = 0;
    pr.timeout = 0;

    connective_ *cc = (connective_*)c;
    cc->pairs = array_append(cc->pairs, &pr);
    return array_tail(cc->pairs);
}

static int connective_send_(connective *c,
                const struct sockaddr *local,
                const struct sockaddr *remote,
                int type) {
    char addr_local[100], addr_remote[100];
    int port_local, port_remote;
    net_inet_ntop(local, addr_local, sizeof(addr_local));
    net_inet_ntop(remote, addr_remote, sizeof(addr_remote));
    port_local = net_address_port(local);
    port_remote = net_address_port(remote);
    log_trace("send %s. Local{%s:%d} ----> Remote{%s:%d}\n",
                    pack_type_string[type],
                    addr_local, port_local,
                    addr_remote, port_remote);

    connective_ *cc = (connective_*)c;
    char buf[128] = {0};
    buf[0] = type;
    return cc->events.on_send_data(c, local, remote, buf, 1);
}

static int connective_send_req_(connective *c,
                const struct sockaddr *local,
                const struct sockaddr *remote) {
    return connective_send_(c, local, remote, p_req);
}

static int connective_send_resp_(connective *c,
                const struct sockaddr *local,
                const struct sockaddr *remote) {
    return connective_send_(c, local, remote, p_resp);
}

static int connective_send_ping_req_(connective *c,
                const struct sockaddr *local,
                const struct sockaddr *remote) {
    return connective_send_(c, local, remote, p_ping_req);
}

static int connective_send_ping_resp_(connective *c,
                const struct sockaddr *local,
                const struct sockaddr *remote) {
    return connective_send_(c, local, remote, p_ping_resp);
}

/* Private functions end */

/* APIs */
connective* connective_create() {
    connective_ *c = malloc(sizeof(connective_));
    c->pairs = array_create(sizeof(pair_));
    c->events.on_send_data = 0;
    c->events.on_state_change = 0;
    c->events.on_timeout = 0;
    return (connective*)c;
}

void connective_attach_events(connective *c, connective_events events) {
    connective_ *cc = (connective_*)c;
    cc->events = events;
}

void connective_destroy(connective *c) {
    connective_ *cc = (connective_*)c;
    array_destroy(cc->pairs);
    free(c);
}

int connective_create_check(connective *c,
                const struct sockaddr *local, 
                const struct sockaddr *remote,
                int *ifexists) {
    int hit = 0;
    pair_ *pr = connective_lookup_(c, local, remote);
    if(!pr) {
        pr = connective_add_pair_(c, local, remote);
    } else {
        hit = 1;
    }

    if( ifexists ) {
        *ifexists = (hit==1?1:0);
    }
    return 0;
}

int connective_remove_check(connective *c,
                const struct sockaddr *local,
                const struct sockaddr *remote) {
    connective_ *cc = (connective_*)c;
    socklen_t addr_len1, addr_len2;
    for(int i=0; i<cc->pairs->size; i++) {
        pair_* pr = array_at(cc->pairs, i);
        if( local->sa_family!=pr->local.sa_family
                        || remote->sa_family!=pr->remote.sa_family )
            continue;

        addr_len1 = net_address_len(local);
        addr_len2 = net_address_len(remote);
        if( memcmp(local, &pr->local, addr_len1)!=0 
                        || memcmp(remote, &pr->remote, addr_len2)!=0 )
            continue;
        
        array_remove(cc->pairs, i);
        return 1;
    }
    return 0;
}

int connective_on_recv_data(connective *c, 
                const struct sockaddr *local, 
                const struct sockaddr *remote,
                const char *data, 
                int data_len) {
    int type = data[0];

    char addr_local[100], addr_remote[100];
    int port_local, port_remote;
    net_inet_ntop(local, addr_local, sizeof(addr_local));
    net_inet_ntop(remote, addr_remote, sizeof(addr_remote));
    port_local = net_address_port(local);
    port_remote = net_address_port(remote);
    log_trace("on_packet(%s) Local{%s:%d} ----> Remote{%s:%d}\n",
                    pack_type_string[type],
                    addr_local, port_local,
                    addr_remote, port_remote);
    pair_ *pr = connective_lookup_(c, local, remote);
    if( type==p_resp ) {
        /* Aggressive check, response from other side */
        if( pr ) {
            if( pr->l_state==s_req_sent ) {
                pr->l_state = s_done;
                pr->state_changed = 1;
            }
        }
        /* Else if !pr, p_resp attack? */
    } 
    if( type==p_req ) {
        /* Passive check */
        if( !pr ) {
            /* Passive add check */
            pr = connective_add_pair_(c, local, remote);
        }
        if( pr->r_state==s_init ) {
            pr->r_state = s_req_recvd;
            pr->state_changed = 1;
        } else if( pr->r_state==s_done ) {
            /* Indicate to send response */
            pr->insist_resp = 1;
        }
    } 
    
    return 0;
}

int connective_drive(connective *c) {
    connective_ *cc = (connective_*)c;
    int events = 0;
    time_t now = time(0);
    for(int i=0 ;i<cc->pairs->size; i++) {
        pair_ *pr = array_at(cc->pairs, i);

        /* l_state */
        if( pr->l_state==s_init ) {
            /* Launch aggressive check */
            connective_send_req_(c, &pr->local, &pr->remote);
            pr->l_state = s_req_sent;
            pr->last_req_time = now;
            pr->first_req_time = now;

            pr->state_changed = 1;
            events++;
        }
        if( pr->l_state==s_req_sent ) {
            if(now-pr->first_req_time>=request_timeout_) {
                /* Aggressive check timeout */
                pr->timeout = 1;
            } else if(now-pr->last_req_time>=request_internal_) {
                /* Retry aggressive check on internal */
                connective_send_req_(c, &pr->local, &pr->remote);
                pr->last_req_time = now;
                events++;
            }
        }

        /* r_state */
        if( pr->r_state==s_req_recvd ) {
            connective_send_resp_(c, &pr->local, &pr->remote);
            pr->r_state = s_done;

            pr->state_changed = 1;
            events++;
        }
        if( pr->r_state==s_done && pr->insist_resp==1 ) {
            /* Last response */
            connective_send_resp_(c, &pr->local, &pr->remote);
            pr->insist_resp = 0;

            events++;
        }

        /* Ping */
        /*if( pr->l_state==s_done &&
                        pr->r_state==s_done &&
                        now-pr->last_ping_time>ping_internal_ ) {
            connective_send_ping_req_(c, &pr->local, &pr->remote);
            events++;
        }*/
        
        if( pr->state_changed==1 ) {
            cc->events.on_state_change(c, &pr->local, &pr->remote, pr->l_state, pr->r_state);
            pr->state_changed = 0;
        }
    }

    /* Remove timeout */
    for(int i=0 ;i<cc->pairs->size; ) {
        pair_ *pr = array_at(cc->pairs, i);
        if( !pr->timeout ) {
            i++;
            continue;
        }
        cc->events.on_timeout(c, &pr->local, &pr->remote);
        array_remove(cc->pairs, i);
    }
    return events;
}

void connective_print_status(connective *c) {
    connective_ *cc = (connective_*)c;
    char addr_local[100], addr_remote[100];
    int port_local, port_remote;
    log_debug("======== [Connective check status] ==========\n");
    log_debug("======== %d pairs =========\n", cc->pairs->size);
    for(int i=0; i<cc->pairs->size; i++) {
        pair_* pr = array_at(cc->pairs, i);

        net_inet_ntop(&(pr->local), addr_local, sizeof(addr_local));
        net_inet_ntop(&(pr->remote), addr_remote, sizeof(addr_remote));
        port_local = net_address_port(&pr->local);
        port_remote = net_address_port(&pr->remote);

        log_debug("Pair[%d] Local{%s:%d}-->Remote{%s:%d}, l_state: %s, r_state: %s.\n",
                        i, addr_local, port_local, addr_remote, port_remote,
                        connective_state_string[pr->l_state],
                        connective_state_string[pr->r_state]);
    }
}

/* APIs end */
