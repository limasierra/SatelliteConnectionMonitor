#include "esno_monitor.h"

static int validity_check(int cnt, struct mon_state *state);
static void execute_alarm_script(int flags, char *rx, char *ns);
static void *worker_thread(void *carry);

/**
 * Initialize the subsystem to raise alarms if e.g. the EsNo threshold
 * has been reached. This complex init is due to a bad structure of the
 * RX / NS index (which is hierarchical, not linear). We just rearrange
 * the hierarchical data into our own linear structures here.
e*/
void mon_state_init(struct mon_state *state, struct rx_index *rx_idx)
{
	state->total = rx_idx->ns_rx_total;
	state->curr = 0;
	state->ns = malloc(state->total * sizeof(struct net_segment *));
	state->rx_for_ns = malloc(state->total * sizeof(size_t));

	// Copy pointers to all the net segments to our ns array
	int rx1_total, rx2_total;
	rx1_total = rx_idx->ns_idx[0].total;
	rx2_total = rx_idx->ns_idx[1].total;

	struct net_segment *rx1_list, *rx2_list;
	rx1_list = rx_idx->ns_idx[0].ns;
	rx2_list = rx_idx->ns_idx[1].ns;

	struct net_segment **pos;
	pos = state->ns;
	for (int i = 0; i < rx1_total; ++i)
		pos[i] = &rx1_list[i];
	pos += rx1_total;
	for (int i = 0; i < rx2_total; ++i)
		pos[i] = &rx2_list[i];

	// Fill reference struct for which NS is on what RX
	size_t *pos2;
	pos2 = state->rx_for_ns;
	for (int i = 0; i < rx1_total; ++i)
		pos2[i] = RX1;
	pos2 += rx1_total;
	for (int i = 0; i < rx2_total; ++i)
		pos2[i] = RX2;
}

/**
 * Free memory
 */
void mon_state_destroy(struct mon_state *state)
{
	free(state->ns);
	free(state->rx_for_ns);
}

/**
 * Check validity of received data (i.e. "is there enough data")-
 *
 * @return 1 if valid, 0 if not
 */
static int validity_check(int cnt, struct mon_state *state)
{
	int total, min;

	total = (int)state->total;
	min = MON_OBSERVATION_TIME / (total * SDD_TIME_SLICE);
	min = min - (0.1 * min);  // threshold

	if (cnt < min)
		return 0;
	return 1;
}

/**
 * Helper function to execute alarm shell script
 */
static void execute_alarm_script(int flags, char *rx, char *ns)
{
	int rv;
	char *exe_cmd;

	if ((rv = asprintf(&exe_cmd, "./%s '%s' '%s' %d", MON_ALARM_EXE,
		      rx, ns, flags)) == -1) {
		fprintf(stderr, "Monitor failed to allocate space for the "
		                "alarm command string!\n");
		return;

	}

	if ((rv = system(exe_cmd)) != 0) {
		fprintf(stderr, "Error while executing alarm script! "
				"Command <%s> returned %d!\n", exe_cmd, rv);
	}

	free(exe_cmd);
}

/**
 * Fetch average EsNo + packet count from database, check if everything
 * is in it's designated limits and finally call the alarm script
 */
static void *worker_thread(void *carry)
{
	mongoc_collection_t *dbc;
	struct mon_state *state;

	// Unpack carry
	dbc = ((struct ev_carry_mon *)carry)->dbc;
	state = &((struct ev_carry_mon *)carry)->state;

	// Bootstrap: Select target NS
	struct net_segment *ns;
	char rx_name[4];

	ns = state->ns[state->curr];
	switch (state->rx_for_ns[state->curr]) {
	case RX1: strncpy(rx_name, "RX1", 4); break;
	case RX2: strncpy(rx_name, "RX2", 4); break;
	}

	// Get recent EsNo averag & entry count from db
	time_t ts_begin;
	double esno_avg;
	int doc_count;
	ts_begin = time(NULL) - MON_OBSERVATION_TIME;
	if (!db_get_esno_avg(dbc, rx_name, ns->name, ts_begin, &esno_avg, &doc_count)) {
		fprintf(stderr, "EsNo monitor: Error at database request!\n");
		exit(EXIT_FAILURE);
	}

	// Perform validity check
	unsigned char flag_validity_check;
	flag_validity_check = 0;
	if (!validity_check(doc_count, state)) {
		printf("Validity check failed: Only have %d packets "
		       "for %d seconds!\n", doc_count,
		       MON_OBSERVATION_TIME);
		flag_validity_check = 1 << 0;
	}

	// Compare with configured EsNo threshold
	double esno_threshold;
	unsigned char flag_esno_threshold;
	esno_threshold = (double) ns->alarm;
	flag_esno_threshold = 0;
	if (esno_avg < esno_threshold) {
		printf("EsNo threshold reached: %f < %f.\n",
		       esno_avg, esno_threshold);
		flag_esno_threshold = 1 << 1;
	}

	// Call the script. The flags will indicate the observations
	int flags;

	flags = flag_validity_check + flag_esno_threshold;
	execute_alarm_script(flags, rx_name, ns->name);

	if (flags != 0) {
		printf("Alarm raised for %s on %s!\n", ns->name, rx_name);
	}

	// Finalize: Adapt monitor state etc
	state->curr = (state->curr + 1) % state->total;

	pthread_exit(NULL);
}

/**
 * Callback for LibEvent timer: Periodically check for long-term EsNo
 * degradation in a round-robin fashion. Spawn a thread to do the work
 * as it may consume enough time to disturb the other event callbacks.
 */
void cb_esno_degradation_monitor(evutil_socket_t fd, short events, void *carry)
{
	pthread_t thread;
	pthread_attr_t attr;
	int rc;
	void *status;

	// Start worker thread
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	rc = pthread_create(&thread, NULL, worker_thread, carry);
	if (rc) {
		fprintf(stderr, "Error spawning thread. Code: %d.\n", rc);
		exit(EXIT_FAILURE);
	}

	pthread_attr_destroy(&attr);

	// Join the thread when it's done
	rc = pthread_join(thread, &status);
	if (rc) {
		fprintf(stderr, "Error return code from thread: %d.\n", rc);
		exit(EXIT_FAILURE);
	}
	if (status != 0) {
		fprintf(stderr, "Thread joined with status %ld.\n",
		                (long)status);
	}
}
