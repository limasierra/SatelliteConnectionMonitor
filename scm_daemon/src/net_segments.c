#include "net_segments.h"

static void ns_add(struct rx_index *rx_idx, size_t rx_id, char *name,
                   char *freq, float alarm);
static char *string_trim(char *str);
static int is_empty_string(const char *str);
static int parse_ns_config_file(struct rx_index *rx_idx, const char *filename);

/**
 * Initialize network segments: Parse config file and set up structs
 */
void rx_index_init(struct rx_index *rx_idx, struct snmp_sessions *snmp_sess)
{
	// Structure:
	// Top level is a struct for the two RX'es (rx_idx)
	// Each RX has a network segments index (ns_idx)
	// These ns index structs each have a list of available network segments (ns)
	// Additionally, the rx_idx has a list of the next RX'es to take.
	// This is due to an algorithm switch: We switch round-robin now.

	// Init rx index
	rx_idx->current = RX1;
	rx_idx->ns_rx_total = 0;
	rx_idx->ns_rx_curr = 0;
	rx_idx->ns_rx_list = NULL;
	rx_idx->snmp_sess = snmp_sess;

	// Init the ns indexes
	for (int i = 0; i < 2; ++i) {
		struct ns_index *ns_idx;
		ns_idx = &rx_idx->ns_idx[i];

		ns_idx->current = RX1;
		ns_idx->total = 0;
		ns_idx->ns = NULL;
	}

	// Now that the structs are ready, add network segments
	if (parse_ns_config_file(rx_idx, NS_CONFIG_FILE) < 1) {
		fprintf(stderr, "No network segments have been configured!\n");
		exit(EXIT_FAILURE);
	}

	// Check if at least one RX is configured and act appropriately
	if (rx_idx->ns_idx[RX1].total < 1) {
		if (rx_idx->ns_idx[RX2].total < 1) {
			fprintf(stderr, "No network segments have been configured!\n");
			exit(EXIT_FAILURE);
		}
		rx_idx->current = RX2;
	}
	snmp_set_active_rx(snmp_sess, rx_idx->current);

	// Activate respective profiles on the device via SNMP
	for (int i = 0; i < 2; ++i) {
		if (rx_idx->ns_idx[i].current > 0) {
			snmp_activate_default_profile(snmp_sess, i);
		}
	}
}

/**
 * Switching algorithm: Proceed to next target network segment.
 * Idea: Switch round-robin through the list of network segments as
 * given in the config file.
 *
 * @return array index of respective NS index. This information is not
 * sufficient for the function caller, but it may be convenient.
 */
size_t ns_take_next(struct rx_index *rx_idx)
{
	size_t curr_rx;
	size_t next_rx;
	size_t next_rx_idx;
	struct ns_index *ns_idx;
	struct net_segment *next_ns;
	struct snmp_sessions *ss;
	char *freq;

	// Get appropriate RX
	curr_rx = rx_idx->current;
	next_rx_idx = (rx_idx->ns_rx_curr + 1) % rx_idx->ns_rx_total;
	next_rx = rx_idx->ns_rx_list[next_rx_idx];

	// Get next NS
	ns_idx = &rx_idx->ns_idx[next_rx];
	ns_idx->current = (ns_idx->current + 1) % ns_idx->total;
	next_ns = &ns_idx->ns[ns_idx->current];

	// Send SNMP frequency switch command
	ss = rx_idx->snmp_sess;
	freq = next_ns->freq;
	snmp_set_freq(ss, next_rx, freq);

	// Do we need to switch the RX too?
	if (curr_rx != next_rx) {
		snmp_set_active_rx(ss, next_rx);
	}

	// Update local bookkeeping
	rx_idx->current = next_rx;
	rx_idx->ns_rx_curr = next_rx_idx;

	// Return new NS array position of respective ns_idx
	return ns_idx->current;
}

/**
 * Unused, old switching algorithm (the reason for the now sub-optimal RX/NS
 * data structures). Included in case it turns out it is still useful.
 * Idea: Always switch to other RX if possible, and prepare next profile on
 * unused RX. Turns out we do _not_ get better lock speeds with this mechanism.
 *
 * @return array index of respective NS index. This information is not
 * sufficient for the function caller, but it may be convenient.
 */
size_t ns_take_next_old(struct rx_index *rx_idx)
{
	size_t curr_rx;
	size_t next_rx;
	struct ns_index *next_ns_idx;
	struct net_segment *next_ns;

	// Decide which RX to take next
	curr_rx = rx_idx->current;
	next_rx = curr_rx ^ 1;  // xor
	next_ns_idx = &rx_idx->ns_idx[next_rx];

	if (next_ns_idx->total < 1) {
		// This RX is not set up! Switch back
		next_rx = curr_rx;
		next_ns_idx = &rx_idx->ns_idx[curr_rx];
	}

	// Update local bookkeeping
	rx_idx->current = next_rx;
	next_ns_idx->current = (next_ns_idx->current + 1) % next_ns_idx->total;
	next_ns = &next_ns_idx->ns[next_ns_idx->current];

	// Send update command to device using SNMP
	struct snmp_sessions *ss = rx_idx->snmp_sess;
	size_t target_rx = next_rx;
	size_t target_ns = next_ns_idx->current;
	const char *freq = next_ns->freq;
	snmp_set_freq(ss, target_rx, freq);

	if (curr_rx != next_rx) {
		// If RX has been switched
		snmp_set_active_rx(ss, next_rx);
	}

	return next_ns_idx->current;
}

/**
 * Get name of network segment, identified by [rx, ns] pair.
 *
 * @return Reference to the given NS name
 */
const char *ns_get_name(struct rx_index *rx_idx, size_t rx, size_t ns)
{
	struct ns_index *ns_idx;

	ns_idx = &rx_idx->ns_idx[rx];
	return ns_idx->ns[ns].name;
}

/**
 * Free allocated memory
 */
void rx_index_free(struct rx_index *rx_idx)
{
	free(rx_idx->ns_idx[RX1].ns);
	free(rx_idx->ns_idx[RX2].ns);
	free(rx_idx->ns_rx_list);
}

/**
 * Helper function to add a network segment to the appropriate RX/NS index
 */
static void ns_add(struct rx_index *rx_idx, size_t rx_id, char *name,
                   char *freq, float alarm)
{
	struct ns_index *ns_idx;
	struct net_segment *this_ns;

	ns_idx = &rx_idx->ns_idx[rx_id];

	// Add NS to appropriate ns_index
	++ns_idx->total;
	if (!(ns_idx->ns = realloc(ns_idx->ns,
	                           ns_idx->total * sizeof(struct net_segment)))) {
		fprintf(stderr, "Failed to realloc memory for NS init!\n");
		exit(EXIT_FAILURE);
	}
	this_ns = ns_idx->ns + ns_idx->total - 1;

	strncpy(this_ns->freq, freq, 12);
	strncpy(this_ns->name, name, 256);
	this_ns->alarm = alarm;

	// Add RX number to a list in rx_index, for easy round-robin switching
	++rx_idx->ns_rx_total;
	if (!(rx_idx->ns_rx_list = realloc(rx_idx->ns_rx_list,
	                             rx_idx->ns_rx_total * sizeof(size_t)))) {
	       fprintf(stderr, "Failed to realloc memory for NS-RX list!\n");
	       exit(EXIT_FAILURE);
	}
	rx_idx->ns_rx_list[rx_idx->ns_rx_total - 1] = rx_id;
}

/**
 * Helper function to trim a string from _both_ sides.
 *
 * @return The new beginning of the string
 */
static char *string_trim(char *str)
{
	if (str == NULL) return str;

	char *end;

	while (isspace(*str)) ++str;

	if (*str == 0) return str;

	end = str + strlen(str) - 1;
	while (end > str && isspace(*end)) --end;

	*(end + 1) = 0;

	return str;
}

/**
 * Helper to check if a string is NULL or empty
 *
 * @return 0 if not empty, else 1
 */
static int is_empty_string(const char *str)
{
	if (str == NULL || strlen(str) < 1)
		return 1;
	return 0;
}

/**
 * Parser for the config file. Also calls the add function to insert parsed
 * config in our structs.
 *
 * @return Number of added configurations
 */
static int parse_ns_config_file(struct rx_index *rx_idx, const char *filename)
{
	FILE *file;
	int count;
	int rx;
	char *ns;
	char *freq;
	char *line;
	float alarm;

	if (!(file = fopen(filename, "r"))) {
		perror("Could not open NS config file");
		exit(EXIT_FAILURE);
	}

	const int linebuf_len = 1000 * sizeof(char);
	char * const linebuf = malloc(linebuf_len);

	count = 0;
	while (fgets(linebuf, linebuf_len, file)) {
		line = string_trim(linebuf);

		// Ignore comment lines
		if (strchr("#\n", *line) != NULL)
			continue;

		if (sscanf(line, " %*[^0-9\n]%d , %m[^,\n] , %m[0-9] , %f \n",
	                   &rx, &ns, &freq, &alarm) != 4) {
			fprintf(stderr, "Erroneous line in config file!\n"
			                "'%s'\n", line);
			exit(EXIT_FAILURE);
		}

		// Trim trailing space
		ns = string_trim(ns);
		freq = string_trim(freq);

		// Check length
		if (is_empty_string(ns)) {
			fprintf(stderr, "Config: NS name is null!\n");
			exit(EXIT_FAILURE);
		}
		if (is_empty_string(freq)) {
			fprintf(stderr, "Config: NS name is null!\n");
			exit(EXIT_FAILURE);
		}

		// Check RX
		--rx;
		if (rx != RX1 && rx != RX2) {
			fprintf(stderr, "Config: Invalid RX given (RX%d)!\n", rx);
			exit(EXIT_FAILURE);
		}

		// Add network segment
		ns_add(rx_idx, rx, ns, freq, alarm);
		++count;

		// Print parsed config
		printf("Config: Added '%s' on 'RX%d' with frequency '%s' "
		       "and alarm threshold '%.2f'.\n", ns, rx + 1, freq, alarm);

		// Free buffers, allocated by sscanf
		free(ns);
		free(freq);
	}

	printf("Config: Parsing done, added %d targets.\n", count);

	free(linebuf);
	fclose(file);

	return count;
}

