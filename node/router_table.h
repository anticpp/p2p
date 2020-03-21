#ifndef P2P_ROUTE_TABLE_H
#define P2P_ROUTE_TABLE_H

#include "router.h"

#define P2P_ROUTE_TABLE_SIZE 1000;

typedef struct {
    p2p_router_t table[P2P_ROUTE_TABLE_SIZE];
    int table_idx;
} p2p_route_table_t;

p2p_route_table_t* p2p_route_table_create();

int p2p_route_table_destroy(p2p_route_table_t *table);

#endif
