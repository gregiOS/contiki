/**
 * \file
 *         An implementation of RPL's objective function proposed by me
 *
 * \author Grzegorz Przybyła
 */

#include "net/rpl/rpl-private.h"
#include "net/nbr-table.h"

#define DEBUG DEBUG_FULL
#include "net/uip-debug.h"

typedef uint16_t rpl_path_metric_t;

static void reset(rpl_dag_t *);
static rpl_parent_t *best_parent(rpl_parent_t *, rpl_parent_t *);
static rpl_dag_t *best_dag(rpl_dag_t *, rpl_dag_t *);
static rpl_rank_t calculate_rank(rpl_parent_t *, rpl_rank_t);
static void update_metric_container(rpl_instance_t *);

#define RPL_DAG_MC_ETX_DIVISIOR 1/2
#define PARENT_SWITCH_THRESHOLD_DIV	2

rpl_of_t rpl_of_gp = {
        reset,
        NULL,
        best_parent,
        best_dag,
        calculate_rank,
        update_metric_container,
        0
};


static void reset(rpl_dag_t *sag) {
    PRINTF("RPL: GP - Objective Function Reset");
}

static rpl_path_metric_t
calculate_path_metric(rpl_parent_t *p)
{
    if(p == NULL) {
        return MAX_PATH_COST * RPL_DAG_MC_ETX_DIVISOR;
    }

#if RPL_DAG_MC == RPL_DAG_MC_NONE
    return p->rank + (uint16_t)p->link_metric;
#elif RPL_DAG_MC == RPL_DAG_MC_ETX
    return p->mc.obj.etx + (uint16_t)p->link_metric;
#elif RPL_DAG_MC == RPL_DAG_MC_ENERGY
  return p->mc.obj.energy.energy_est + (uint16_t)p->link_metric;
#else
#error "Unsupported RPL_DAG_MC configured. See rpl.h."
#endif /* RPL_DAG_MC */
}

static rpl_parent_t *best_parent(rpl_parent_t *p1, rpl_parent_t *p2)
{
    rpl_dag_t *dag;
    rpl_path_metric_t min_diff;
    rpl_path_metric_t p1_metric;
    rpl_path_metric_t p2_metric;

    dag = p1->dag;

    min_diff = RPL_DAG_MC_ETX_DIVISOR /
               PARENT_SWITCH_THRESHOLD_DIV;

    p1_metric = calculate_path_metric(p1);
    p2_metric = calculate_path_metric(p2);

    if(p1 == dag->preferred_parent || p2 == dag->preferred_parent) {
        if(p1_metric < p2_metric + min_diff &&
           p1_metric > p2_metric - min_diff) {
            return dag->preferred_parent;
        }
    }
    if (p1_metric < p2_metric ) {
        return p1->dag->instance->mc.type != RPL_DAG_MC_ENERGY_TYPE_BATTERY ? p1 : p2;
    }
    return p2->dag->instance->mc.type != RPL_DAG_MC_ENERGY_TYPE_BATTERY ? p2 : p1;

}

static rpl_dag_t *best_dag(rpl_dag_t *d1, rpl_dag_t *d2) {
    if(d1->grounded != d2->grounded) {
        return d1->grounded ? d1 : d2;
    }

    if(d1->preference != d2->preference) {
        return d1->preference > d2->preference ? d1 : d2;
    }

    if (d1->rank < d2->rank ) {
        return d1->instance->mc.type != RPL_DAG_MC_ENERGY_TYPE_BATTERY ? d1 : d2;
    }
    return d2->instance->mc.type != RPL_DAG_MC_ENERGY_TYPE_BATTERY ? d2 : d1;


}

static rpl_rank_t calculate_rank(rpl_parent_t *parent, rpl_rank_t base) {
    rpl_rank_t new_rank;
    rpl_rank_t rank_increase;

    if (parent == NULL) {
        if (base == 0) {
            return INFINITE_RANK;
        }
        rank_increase = RPL_INIT_LINK_METRIC * RPL_DAG_MC_ETX_DIVISIOR;
    } else {
        rank_increase = parent->link_metric;
        if (base == 0) {
            base = parent->rank;
        }
    }

    if (INFINITE_RANK - base < rank_increase) {
        new_rank = INFINITE_RANK;
    } else {
        new_rank = base + rank_increase;
    }
    return new_rank;
}

static void update_metric_container(rpl_instance_t *instance)
{
  rpl_path_metric_t path_metric;
  rpl_dag_t *dag;
  uint8_t type;

  instance->mc.type = RPL_DAG_MC;
  instance->mc.flags = RPL_DAG_MC_FLAG_P;
  instance->mc.aggr = RPL_DAG_MC_AGGR_ADDITIVE;
  instance->mc.prec = 0;

  dag = instance->current_dag;

  if (!dag->joined) {
    PRINTF("RPL: Cannot update the metric container when not joined\n");
    return;
  }

  if(dag->rank == ROOT_RANK(instance)) {
    path_metric = 0;
  } else {
    path_metric = calculate_path_metric(dag->preferred_parent);
  }

  instance->mc.length = sizeof(instance->mc.obj.energy);

  if(dag->rank == ROOT_RANK(instance)) {
    type = RPL_DAG_MC_ENERGY_TYPE_MAINS;
  } else {
    type = RPL_DAG_MC_ENERGY_TYPE_SCAVENGING;
  }
#ifdef NODE_BATTERY_POWERED
    type = RPL_DAG_MC_ENERGY_TYPE_BATTERY;
#endif

  instance->mc.obj.energy.flags = type << RPL_DAG_MC_ENERGY_TYPE;
  instance->mc.obj.energy.energy_est = path_metric;
}

