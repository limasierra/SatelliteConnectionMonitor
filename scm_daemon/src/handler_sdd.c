#include "handler_sdd.h"

static inline void fill_sdd_struct(struct sdd_msg *s, unsigned char *buf);
static void check_validity(struct sdd_slice_accumulator *accu, const char *rx_name,
                          const char *ns_name);
static void flush_accumulator(struct sdd_slice_accumulator *accu,
                              mongoc_collection_t *dbc, struct rx_index *rx_idx);

/**
 * Initialize / reset the SDD accumulator. Shall be called for each network
 * segment change. The accumulator is a struct that allows the SDD handler to
 * be stateful, carrying information over multiple invocations and thus
 * allowing the calculation of e.g. an average
 */
void reset_sdd_accu(struct sdd_slice_accumulator *accu, size_t rx, size_t ns)
{
	accu->rx = rx;
	accu->ns = ns;
	accu->esno_sum = 0;
	accu->count = 0;
	accu->count_bad = 0;
	accu->count_total = 0;
	accu->valid_flag = 1;
	accu->since_ts = time(NULL);
}

/**
 * Helper to parse the SDD message. Takes the data from the appropriate offsets
 * and puts it into our message struct.
 */
static inline void fill_sdd_struct(struct sdd_msg *s, unsigned char *buf)
{
	s->esno = (buf[150] << 8) + buf[151];
	s->demod_locked = (buf[6] >> 4) & 0x1;
	s->demod_tracked = (buf[6] >> 6) & 0x1;
	s->lock_definitive = (buf[6] >> 7) & 0x1;
}

/**
 * Check validity of accumulator before flushing it into database. The
 * accu contains a 'valid_flag', which is read/set here.
 */
static void check_validity(struct sdd_slice_accumulator *accu, const char *rx_name,
                          const char *ns_name)
{
	int total = accu->count_total;
	int bad = accu->count_bad;

	if (accu->valid_flag == 0) {
		printf("SDD handler: Timeout detected for %s on %s!\n"
		       "Discarding data... Check the configuration!\n",
		       ns_name, rx_name);
		return;
	}

	if ((1.0 * total) / bad < 1.5) {
		printf("SDD handler: Discarded presumably bad (%d/%d) data!\n"
		       "Check if %s on %s is correctly configured!\n",
		       bad, total, ns_name, rx_name);
		accu->valid_flag = 0;
		return;
	}
}

/**
 * Accumulator shall be flushed to database: Check accu validity, call database
 * insert function and proceed to next network segment
 */
static void flush_accumulator(struct sdd_slice_accumulator *accu,
                              mongoc_collection_t *dbc, struct rx_index *rx_idx)
{
	size_t rx, ns;
	double avg_esno;
	const char *ns_name;

	// Get name of RX and NS
	rx = accu->rx;
	ns = accu->ns;
	ns_name = ns_get_name(rx_idx, rx, ns);

	char rx_name[4];
	switch (rx) {
	case RX1: strncpy(rx_name, "RX1", 4); break;
	case RX2: strncpy(rx_name, "RX2", 4); break;
	}

	// Check packet validity
	check_validity(accu, rx_name, ns_name);

	if (!accu->valid_flag) {
		printf("Packet failed validity check! Setting EsNo to zero.\n");
	}

	// Calculate average EsNo
	if (accu->count == 0 || !accu->valid_flag)
		avg_esno = 0;
	else
		avg_esno = accu->esno_sum / (accu->count * 10.0);

	// printf("EsNo average for %s on %s of %d values over %d seconds: %f\n\n",
	//        ns_name, rx_name, accu->count, SDD_TIME_SLICE, avg_esno);

	// Insert into database
	db_insert_sdd(dbc, rx, ns_name, accu->since_ts, avg_esno);

	// Proceed to next network segment
	size_t new_ns = ns_take_next(rx_idx);
	size_t new_rx = rx_idx->current;
	reset_sdd_accu(accu, new_rx, new_ns);
}

/**
 * Callback for LibEvent when SDD message is received. Add info to accumulator
 * and flush the accu to the database if needed.
 */
void cb_recv_sdd_packet(evutil_socket_t fd, short events, void *carry)
{
	static unsigned char buf[SDD_BUFSIZ];
	struct sockaddr_storage remote_addr;
	struct sdd_slice_accumulator *accu;
	struct sdd_msg sdd_msg;
	mongoc_collection_t *dbc;
	struct rx_index *rx_idx;
	int numbytes;

	// Unpack carry
	accu = &((struct ev_carry_sdd *)carry)->accu;
	dbc = ((struct ev_carry_sdd *)carry)->dbc;
	rx_idx = ((struct ev_carry_sdd *)carry)->rx_idx;

	// Get UDP packet
	numbytes = get_udp_packet(fd, buf, SDD_BUFSIZ, &remote_addr);

	// Handle two cases:
	// 1. Normal: incoming packets are processed
	// 2. Timeout: No packets during some period of time: "null" EsNo!
	if (numbytes < 0) {
		// Flush to database
		accu->valid_flag = 0;
		flush_accumulator(accu, dbc, rx_idx);

	} else {
		// For stats, count every received packet
		accu->count_total++;

		// Ignore first few seconds, as packets from previous NS
		// might come through
		time_t curr_ts = time(NULL);
		if (curr_ts - accu->since_ts < 1)
			return;

		// Fill message into struct
		fill_sdd_struct(&sdd_msg, buf);

		// Only take if demod is locked
		if (sdd_msg.demod_locked != 0x1) {
			accu->count_bad++;
			return;
		}

		// Filter out extremely high values
		if (sdd_msg.esno > 0xF00) {
			accu->count_bad++;
			return;
		}

		// Add current EsNo to accumulator
		accu->count++;
		accu->esno_sum += sdd_msg.esno;

		// Check if we need to flush the current accumulator to database
		if (curr_ts - accu->since_ts - SDD_TIME_SLICE >= 0) {
			flush_accumulator(accu, dbc, rx_idx);
		}
	}
}

