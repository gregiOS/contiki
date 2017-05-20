//
// Created by Grzegorz Przybyła on 20/05/2017.
//

#include "rpl-non-storing.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "net/rpl/rpl-private.h"
#include "net/ip/uip.h"
#include "net/rpl/rpl-conf.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/tcpip.h"
#include "net/ipv6/uip-icmp6.h"
#include "net/ip/uip-debug.h"
#include <limits.h>
#include <string.h>

#if RPL_WITH_NON_STORING

#define DEBUG DEBUG_FULL

static int number_of_nodes;

LIST(list_of_nodes);
MEMB(mem_node_copy, rpl_non_storing_node_t, RPL_NS_LINK_NUM);

void rpl_non_storing_init(void) {
    list_init(list_of_nodes);
    memb_init(&mem_node_copy);
    number_of_nodes = 0;
}

void rpl_non_storing_get_node_global_addr(uip_ipaddr_t *addr, rpl_non_storing_node_t *node) {
  if(addr != NULL && node != NULL && node->dag != NULL) {
    memcpy(addr, &node->dag->dag_id, 8);
    memcpy(((unsigned char *)addr) + 8, &node->link_identifier, 8);
  }
}

void rpl_non_storing_periodic(void) {
  rpl_non_storing_node_t *l;
  for(l = list_head(list_of_nodes); l != NULL; l = list_item_next(l)) {
    if(l->lifetime != 0xffffffff && l->lifetime > 0) {
      l->lifetime--;
    }
  }
  for(l = list_head(list_of_nodes); l != NULL; l = list_item_next(l)) {
    if(l->lifetime == 0) {
      rpl_non_storing_node_t *l2;
      for(l2 = list_head(list_of_nodes); l2 != NULL; l2 = list_item_next(l2)) {
        if(l2->parent == l) {
          break;
        }
      }
      list_remove(list_of_nodes, l);
      memb_free(&mem_node_copy, l);
      number_of_nodes--;
    }
  }
}

int rpl_non_storing_number_of_nodes(void) {
  return number_of_nodes;
}

static int node_matches_address(const rpl_dag_t *dag, const rpl_non_storing_node_t *node, const uip_ipaddr_t *addr) {
  return addr != NULL
      && node != NULL
      && dag != NULL
      && dag == node->dag
      && !memcmp(addr, &node->dag->dag_id, 8)
      && !memcmp(((const unsigned char *)addr) + 8, node->link_identifier, 8);
}

rpl_non_storing_node_t *rpl_non_storing_get_node(const rpl_dag_t *dag, const uip_ipaddr_t *addr) {
  rpl_non_storing_node_t *l;
  for(l = list_head(list_of_nodes); l != NULL; l = list_item_next(l)) {
    if(node_matches_address(dag, l, addr)) {
      return l;
    }
  }
  return NULL;
}

int rpl_non_storing_is_node_reachable(const rpl_dag_t *dag, const uip_ipaddr_t *addr) {
  int max_depth = RPL_NS_LINK_NUM;
  rpl_non_storing_node_t *node = rpl_non_storing_get_node(dag, addr);
  rpl_non_storing_node_t *root_node = rpl_non_storing_get_node(dag, dag != NULL ? &dag->dag_id : NULL);
  while(node != NULL && node != root_node && max_depth > 0) {
    node = node->parent_node;
    max_depth--;
  }
  return node != NULL && node == root_node;
}

void rpl_non_storing_expire_parent(rpl_dag_t *dag, const uip_ipaddr_t *child, const uip_ipaddr_t *parent)
{
  rpl_non_storing_node_t *l = rpl_non_storing_get_node(dag, child);
  if(l != NULL && node_matches_address(dag, l->parent, parent)) {
    l->lifetime = RPL_NOPATH_REMOVAL_DELAY;
  }
}

rpl_non_storing_node_t *rpl_non_storing_update_node(rpl_dag_t *dag, const uip_ipaddr_t *child, const uip_ipaddr_t *parent, uint32_t lifetime)
{
  rpl_non_storing_node_t *child_node = rpl_non_storing_get_node(dag, child);
  rpl_non_storing_node_t *parent_node = rpl_non_storing_get_node(dag, parent);
  rpl_non_storing_node_t *old_parent_node;

  if(parent != NULL) {
    if(parent_node == NULL) {
      parent_node = rpl_non_storing_update_node(dag, parent, NULL, 0xffffffff);
      if(parent_node == NULL) {
        return NULL;
      }
    }
  }

  if(child_node == NULL) {
    child_node = memb_alloc(&mem_node_copy);
    if(child_node == NULL) {
      return NULL;
    }
    child_node->parent = NULL;
    list_add(list_of_nodes, child_node);
    number_of_nodes++;
  }

  child_node->dag = dag;
  child_node->lifetime = lifetime;
  memcpy(child_node->link_identifier, ((const unsigned char *)child) + 8, 8);

  if(rpl_non_storing_is_node_reachable(dag, child)) {
    old_parent_node = child_node->parent_node;
    child_node->parent_node = parent_node;
    if(!rpl_non_storing_is_node_reachable(dag, child)) {
      child_node->parent = old_parent_node;
    }
  } else {
    child_node->parent = parent_node;
  }

  return child_node;
}



rpl_non_storing_node_t *rpl_non_storing_node_head(void) {
  return list_head(list_of_nodes);
}

rpl_non_storing_node_t *rpl_non_storing_node_next(rpl_non_storing_node_t *item) {
  return list_item_next(item);
}
