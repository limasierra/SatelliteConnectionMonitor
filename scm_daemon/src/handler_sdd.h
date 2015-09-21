#ifndef HANDLER_SDD_H
#define HANDLER_SDD_H

#include "common.h"

// Holds (relevant) information from the SDD messages
struct sdd_msg {
	unsigned char lock_definitive : 1;
	unsigned char demod_tracked : 1;
	unsigned char demod_locked : 1;
	uint16_t esno;
};

// The accumulator: Preserve state of the handler
struct sdd_slice_accumulator {
	size_t rx;
	size_t ns;
	int esno_sum;
	int count;
	int count_bad;
	int count_total;
	unsigned char valid_flag;
	time_t since_ts;
};

// Carry for LibEvent callback
struct ev_carry_sdd {
	struct sdd_slice_accumulator accu;
	mongoc_collection_t *dbc;
	struct rx_index *rx_idx;
};

void reset_sdd_accu(struct sdd_slice_accumulator *accu, size_t rx, size_t ns);
void cb_recv_sdd_packet(evutil_socket_t fd, short events, void *carry);

#endif // HANDLER_SDD_H
