#ifndef NET_SEGMENTS_H
#define NET_SEGMENTS_H

#include "common.h"

enum { RX1 = 0, RX2 = 1 };

// One actual network segment
struct net_segment {
	char freq[12];
	char name[256];
	float alarm;  // Threshold
};

// Index of network segments for one RX
struct ns_index {
	size_t current;
	size_t total;
	struct net_segment *ns;
};

// The 'main' container
struct rx_index {
	size_t current;  // RX1 or RX2
	struct ns_index ns_idx[2];
	size_t ns_rx_total;
	size_t ns_rx_curr;  // index in ns_rx_list array
	size_t *ns_rx_list;
	struct snmp_sessions *snmp_sess;
};

void rx_index_init(struct rx_index *rx_idx, struct snmp_sessions *snmp_sess);
size_t ns_take_next(struct rx_index *rx_idx);
const char *ns_get_name(struct rx_index *rx_idx, size_t rx, size_t ns);
void rx_index_free(struct rx_index *rx_idx);

#endif // NET_SEGMENTS_H
