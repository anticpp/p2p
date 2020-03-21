#ifndef __CONN_H__
#define __CONN_H__

#include <sys/socket.h>

/*
 *  4-way-handshake
 *
 *  L                            R
 *  -                            -
 *  Request ->                      \  L's
 *                <-      Response  /  Check
 *                <-      Request   \  R's
 *  Response ->                     /  Check
 *
 * */

typedef enum {
    s_init = 0,
    s_req_sent,
    s_req_recvd,
    s_done,
} connective_state;

const static char* connective_state_string[] = {
    "s_init",
    "s_req_sent",
    "s_req_recvd",
    "s_done"
};

typedef struct {
    char _;
} connective;

typedef int (*connective_on_send_data) (connective* c,
                    const struct sockaddr *local,
                    const struct sockaddr *remote, 
                    const char *data,
                    int data_len);

typedef int (*connective_on_state_change) (connective *c,
                    const struct sockaddr *local,
                    const struct sockaddr *remote, 
                    connective_state l_state,
                    connective_state r_state);

typedef int (*connective_on_timeout) (connective *c,
                    const struct sockaddr *local,
                    const struct sockaddr *remote);

typedef struct {
    connective_on_send_data on_send_data;
    connective_on_state_change on_state_change;
    connective_on_timeout on_timeout;
} connective_events;

/* Export APIs */
connective* connective_create();
void connective_attach_events(connective *c, connective_events events);
void connective_destroy(connective *c);
int connective_create_check(connective *c,
                const struct sockaddr *local, 
                const struct sockaddr *remote,
                int *ifexists);
int connective_remove_check(connective *c,
                const struct sockaddr *local,
                const struct sockaddr *remote);
int connective_on_recv_data(connective *c, 
                const struct sockaddr *local, 
                const struct sockaddr* remote,
                const char *data, 
                int data_len);
int connective_drive(connective *c);
void connective_print_status(connective *c);

#endif
