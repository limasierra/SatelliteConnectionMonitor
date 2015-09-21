#ifndef ESNO_MONITOR_H
#define ESNO_MONITOR_H

#include "common.h"

// State holder for the monitor
struct mon_state {
	size_t total;
	size_t curr;
	struct net_segment **ns;
	size_t *rx_for_ns;
};

// Carry for LibEvent callback
struct ev_carry_mon {
	mongoc_collection_t *dbc;
	struct rx_index *rx_idx;
	struct mon_state state;
};

void mon_state_init(struct mon_state *state, struct rx_index *rx_idx);
void mon_state_destroy(struct mon_state *state);
void cb_esno_degradation_monitor(evutil_socket_t fd, short events, void *carry);

#endif // ESNO_MONITOR_H
