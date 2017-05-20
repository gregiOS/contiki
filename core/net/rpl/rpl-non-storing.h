//
// Created by Grzegorz Przybyła on 20/05/2017.
//

#ifndef CONTIKI_RPL_NON_STORING_H
#define CONTIKI_RPL_NON_STORING_H

//RPL non storing mod eimplementation. Need to be implemented to support the source routing

#include "rpl-conf.h"

#ifdef RPL_NS_CONF_LINK_NUM
#define RPL_NS_LINK_NUM RPL_NS_CONF_LINK_NUM
#else
#define RPL_NS_LINK_NUM 16
#endif

typedef struct rpl_non_storing_node {
    unsigned char link_identifier[8];
    uint32_t lifetime;
    rpl_dag_t *dag;
    struct rpl_non_storing_node *parent_node;
    struct rpl_non_storing_node *next_node;
} rpl_non_storing_node_t;

// MARK: - methods
void rpl_non_storing_init(void);
void rpl_non_storing_get_node_global_addr(uip_ipaddr_t *addr, rpl_ns_node_t *node);
void rpl_non_storing_periodic();
void rpl_non_storing_expire_parent(rpl_dag_t *dag, const uip_ipaddr_t *child, const uip_ipaddr_t *parent);

int rpl_non_storing_num_nodes(void);
int rpl_non_storing_is_node_reachable(const rpl_dag_t *dag, const uip_ipaddr_t *addr);
rpl_non_storing_node_t *rpl_non_storing_update_node(rpl_dag_t *dag, const uip_ipaddr_t *child, const uip_ipaddr_t *parent, uint32_t lifetime);

rpl_non_storing_node_t *rpl_non_storing_node_head(void);
rpl_non_storing_node_t *rpl_non_storing_node_next(rpl_ns_node_t *item);
rpl_non_storing_node_t *rpl_non_storing_get_node(const rpl_dag_t *dag, const uip_ipaddr_t *addr);




#endif //CONTIKI_RPL_NON_STORING_H
