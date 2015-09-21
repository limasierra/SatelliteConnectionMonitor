#ifndef HANDLER_MC_H
#define HANDLER_MC_H

#include "common.h"

// The MODCOD accumulator, to preserve the state of the function
struct mc_accu {
	time_t ts;
	uint64_t curr[29]; /* Convention: [0] is total */
	uint64_t old[29];
	uint64_t diff[29];
	uint64_t perc[29];
	uint64_t sum_normal_old;
	uint64_t sum_short_old;
	uint64_t bit_rate;
};

// Carry for LibEvent callback
struct ev_carry_mc {
	struct mc_accu accu;
	mongoc_collection_t *dbc;
};

void init_mc_accu(struct mc_accu *accu);
void cb_recv_mc_packet(evutil_socket_t fd, short events, void *carry);

#endif // HANDLER_MC_H
