#include "handler_mc.h"

static inline uint64_t build_uint64(unsigned char *buf);
static int parse_buf_into_struct(struct mc_accu *accu, unsigned char *buf);
static inline void print_array(struct mc_accu *accu);

// Map MODCOD numbers to their names (Ref: ETSI 302307-1 V1.4.1, 5.5.2.2)
const char *modcod_names[32] = {
	"Total", /* Actually DUMMY PLFRAME, but we don't use it */
	"QPSK 1/4", "QPSK 1/3", "QPSK 2/5", "QPSK 1/2",
	"QPSK 3/5", "QPSK 2/3", "QPSK 3/4", "QPSK 4/5",
	"QPSK 5/6", "QPSK 8/9", "QPSK 9/10", "8PSK 3/5",
	"8PSK 2/3", "8PSK 3/4", "8PSK 5/6", "8PSK 8/9",
	"8PSK 9/10", "16APSK 2/3", "16APSK 3/4", "16APSK 4/5",
	"16APSK 5/6", "16APSK 8/9", "16APSK 9/10", "32APSK 3/4",
	"32APSK 4/5", "32APSK 5/6", "32APSK 8/9", "32APSK 9/10",
	"Reserved (29)", "Reserved (30)", "Reserved (31)"
};

/**
 * Helper to build a 64-bit unsigned int byte-by-byte
 *
 * @return Resulting uint64_t number
 */
static inline uint64_t build_uint64(unsigned char *buf)
{
	return (((uint64_t) buf[0]) << 7 * 8) +
	       (((uint64_t) buf[1]) << 6 * 8) +
	       (((uint64_t) buf[2]) << 5 * 8) +
	       (((uint64_t) buf[3]) << 4 * 8) +
	       (((uint64_t) buf[4]) << 3 * 8) +
	       (((uint64_t) buf[5]) << 2 * 8) +
	       (((uint64_t) buf[6]) << 1 * 8) +
	       (((uint64_t) buf[7]) << 0 * 8);
}

/**
 * Parse the MODCOD stats UDP message and insert data in our struct
 */
static int parse_buf_into_struct(struct mc_accu *accu, unsigned char *buf)
{
	unsigned char is_not_first_measurement = 1;
	time_t ts, ts_old;
	uint64_t *curr;
	uint64_t sum, sum_normal, sum_short;
	uint64_t sum_normal_old, sum_short_old;
	unsigned int time_interval;

	curr = accu->curr;
	ts_old = accu->ts;
	sum = sum_normal = sum_short = 0;
	buf += 40;  // offset

	// Check if it's the first measurement
	if (curr[0] == 0)
		is_not_first_measurement = 0;

	// Get current timestamp
	ts = time(NULL);

	// Process raw buffer and sum up needed data
	for (int i = 1; i < 29; ++i) {
		uint64_t this_modcod;

		this_modcod = 0;
		// Separate normal/short frames for accurate
		// bit rate measurements
		for (int type = 0; type < 2; ++type) {
			uint64_t count = build_uint64(buf);
			this_modcod += count;
			sum_normal += count;
			buf += 8;
		}
		for (int type = 0; type < 2; ++type) {
			uint64_t count = build_uint64(buf);
			this_modcod += count;
			sum_short += count;
			buf += 8;
		}

		sum += this_modcod;
		curr[i] = this_modcod;
	}

	accu->curr[0] = sum;  // array[0] is the total

	// Calculate bit rate
	sum_normal_old = accu->sum_normal_old;
	sum_short_old = accu->sum_short_old;
	time_interval = ts - ts_old;  // second precision
	accu->bit_rate = 64800 * (sum_normal - sum_normal_old) +
	                 16200 * (sum_short - sum_short_old);
	if (time_interval != 0)
		accu->bit_rate /= time_interval;

	// Calculate percentage
	for (int i = 0; i < 29; ++i) {
		accu->perc[i] = (1.0 * curr[i] / curr[0]) * 1000;
	}

	// Calculate diff
	for (int i = 0; i < 29; ++i) {
		accu->diff[i] = curr[i] - accu->old[i];
	}

	// Save old values needed in the next round
	accu->ts = ts;
	memcpy(accu->old, accu->curr, 29 * sizeof(uint64_t));
	accu->sum_normal_old = sum_normal;
	accu->sum_short_old = sum_short;

	return is_not_first_measurement;
}

/**
 * Print outcome in command-line
 */
static inline void print_array(struct mc_accu *accu)
{
	uint64_t bit_rate, bit_rate_hi, bit_rate_comma;
	uint64_t divisor;
	uint64_t *curr, *diff, *perc;

	bit_rate = accu->bit_rate;
	curr = accu->curr;
	diff = accu->diff;
	perc = accu->perc;

	divisor = 1000000;
	bit_rate_hi = bit_rate / divisor;
	bit_rate_comma = bit_rate - (bit_rate_hi * divisor);
	while (bit_rate_comma > 1000)
		bit_rate_comma /= 10;

	printf("%12s: %2llu.%3.3llu Mbit/s\n", "Bit rate", bit_rate_hi, bit_rate_comma);

	for (int i = 0; i < 29; i++) {
		printf("%12s:  %10llu", modcod_names[i], curr[i]);
		if (diff[i] > 0)
			printf("   (%3.1f\%, +%lld)", perc[i] / 10.0, diff[i]);
		else if (perc[i] > 0)
			printf("   (%3.1f\%)", perc[i] / 10.0, diff[i]);
		printf("\n");
	}

	printf("\n");
}

/**
 * Initialize MODCOD accumulator (i.e. the "state holder")
 */
void init_mc_accu(struct mc_accu *accu)
{
	accu->ts = time(NULL);
	memset(accu->curr, 0, 29 * sizeof(uint64_t));
	memset(accu->old, 0, 29 * sizeof(uint64_t));
	memset(accu->diff, 0, 29 * sizeof(uint64_t));
	accu->sum_normal_old = 0;
	accu->sum_short_old = 0;
	accu->bit_rate = 0;
}

/**
 * Callback for LibEvent when MODCOD packet has been received. Parse raw buffer
 * and insert into database
 */
void cb_recv_mc_packet(evutil_socket_t fd, short events, void *carry)
{
	static unsigned char buf[MC_BUFSIZ];
	struct mc_accu *accu;
	struct sockaddr_storage remote_addr;
	mongoc_collection_t *dbc;
	int numbytes;

	// Unpack carry
	accu = &((struct ev_carry_mc *)carry)->accu;
	dbc = ((struct ev_carry_mc *)carry)->dbc;

	// Get UDP packet
	numbytes = get_udp_packet(fd, buf, MC_BUFSIZ, &remote_addr);

	// Fill message into struct
	if (!parse_buf_into_struct(accu, buf)) {
		return;
	}

	// Print values
	//print_array(accu);

	// Insert into database
	db_insert_mc(dbc, accu);
}

