#include "snmplib.h"

static void init_session_helper(netsnmp_session *s, char *community);
static void open_session_helper(netsnmp_session *local, netsnmp_session **remote);
static int snmp_set(netsnmp_session *s, char *oid_str, char type, const char *val);
static int get_profile_activate_oid(size_t rx, unsigned char profile, char *buf);
static int get_freq_tuner_oid(size_t rx, char *buf);

/**
 * Initialize SNMP library
 */
void snmp_init(struct snmp_sessions *ss)
{
	// Initialize SNMP library
	init_snmp("scm_daemon");

	// Initialize local sessions for reading and writing
	init_session_helper(&ss->read_local, "public");
	init_session_helper(&ss->write_local, "private");

	// Open the sessions to get a remote connection
	open_session_helper(&ss->read_local, &ss->read);
	open_session_helper(&ss->write_local, &ss->write);
}

/**
 * Helper for snmp_init
 */
static void init_session_helper(netsnmp_session *s, char *community)
{
	snmp_sess_init(s);
	s->peername = strdup(TC1_IP_ADDR);
	s->version = SNMP_VERSION_2c;
	s->community = (unsigned char *)strdup(community);
	s->community_len = strlen(community);
}

/**
 * Helper for snmp_init
 */
static void open_session_helper(netsnmp_session *local, netsnmp_session **remote)
{
	*remote = snmp_open(local);

	if (!*remote) {
		snmp_sess_perror("SNMP ack", local);
		exit(EXIT_FAILURE);
	}
}

/**
 * The 'raw' SNMP set function. It should not be used directly in external
 * files. Rather, a non-static wrapper function should be created here that
 * makes the appropriate call in order to preserve source code readability.
 *
 * @return 1 if successful, 0 if not
 */
static int snmp_set(netsnmp_session *s, char *oid_str, char type, const char *val)
{
	netsnmp_pdu *pdu;
	oid the_oid[MAX_OID_LEN];
	size_t oid_len;

	pdu = snmp_pdu_create(SNMP_MSG_SET);
	oid_len = MAX_OID_LEN;

	// Parse the OID
	if (snmp_parse_oid(oid_str, the_oid, &oid_len) == 0) {
		snmp_perror(oid_str);
		return 0;
	}

	// Build the packet to be sent
	if (snmp_add_var(pdu, the_oid, oid_len, type, val) != 0) {
		printf("type: %c, val: %s, oid_str: %s\n", type, val, oid_str);
		snmp_perror("SNMP: Could not add var!");
		return 0;
	}

	// Send the request
	if (snmp_send(s, pdu) == 0) {
		snmp_perror("SNMP: Error while sending!");
		snmp_free_pdu(pdu);
		return 0;
	}

	return 1;
}

/**
 * Helper function which puts the appropriate OID in 'buf' for the profile to
 * be activated.
 *
 * @return 1 on success, 0 if not
 */
static int get_profile_activate_oid(size_t rx, unsigned char profile, char *buf)
{
	switch (rx) {
	case RX1:
		if (profile == 0)
			strncpy(buf, SNMP_RX1_CFG1_MODE, 40);
		else if (profile == 1)
			strncpy(buf, SNMP_RX1_CFG2_MODE, 40);
		else
			goto invalidprofile;
		break;
	case RX2:
		if (profile == 0)
			strncpy(buf, SNMP_RX2_CFG1_MODE, 40);
		else if (profile == 1)
			strncpy(buf, SNMP_RX2_CFG2_MODE, 40);
		else
			goto invalidprofile;
		break;
	invalidprofile:
		fprintf(stderr, "SNMP: invalid profile given! (a) %u!\n", profile);
		return 0;
	default:
		fprintf(stderr, "SNMP: invalid RX given! (a) %u!\n", profile);
		return 0;
	}

	return 1;
}

/**
 * Wrapper to set the default active profile
 */
void snmp_activate_default_profile(struct snmp_sessions *ss, size_t rx)
{
	snmp_set_active_profile(ss, rx, TC1_DEFAULT_PROFILE);
}

/**
 * Wrapper to set the given profile as active
 */
void snmp_set_active_profile(struct snmp_sessions *ss, size_t rx, unsigned char profile)
{
	char oid[40];

	if (!get_profile_activate_oid(rx, profile, oid)) {
		fprintf(stderr, "SNMP: Could not get profile OID\n");
		return;
	}

	if (!snmp_set(ss->write, oid, 'i', "0")) {
		fprintf(stderr, "SNMP: Set active profile failed!\n");
	}
}

/**
 * Helper to get the appropriate OID string into 'buf' for the frequency tuner
 *
 * @return 1 on success, 0 if not
 */
static int get_freq_tuner_oid(size_t rx, char *buf)
{
	unsigned char profile = TC1_DEFAULT_PROFILE;

	switch (rx) {
	case RX1:
		if (profile == 0)
			strncpy(buf, SNMP_RX1_CFG1_TUNER, 40);
		else if (profile == 1)
			strncpy(buf, SNMP_RX1_CFG2_TUNER, 40);
		else
			goto invalidsettings;
		break;
	case RX2:
		if (profile == 0)
			strncpy(buf, SNMP_RX2_CFG1_TUNER, 40);
		else if (profile == 1)
			strncpy(buf, SNMP_RX2_CFG2_TUNER, 40);
		else
			goto invalidsettings;
		break;
	invalidsettings:
		fprintf(stderr, "SNMP: invalid profile configured! (b) %u!\n", profile);
		return 0;
	default:
		fprintf(stderr, "SNMP: invalid RX given! (b) %u!\n", profile);
		return 0;
	}

	return 1;
}

/**
 * Wrapper to set the given tuner frequency
 */
void snmp_set_freq(struct snmp_sessions *ss, size_t rx, const char *frequency)
{
	char oid[40];

	if (!get_freq_tuner_oid(rx, oid)) {
		fprintf(stderr, "SNMP: Could not get OID to tune frequency!\n");
		return;
	}

	if (!snmp_set(ss->write, oid, 'u', frequency)) {
		fprintf(stderr, "SNMP: Set frequency failed!\n");
	}
}

/**
 * Wrapper to set the active RX
 */
void snmp_set_active_rx(struct snmp_sessions *ss, size_t rx)
{
	char oid[40];
	char rx_str[2];

	strncpy(oid, SNMP_RX_MGMT, 40);

	if (rx == RX1) {
		strncpy(rx_str, "1", 2);
	} else if (rx == RX2) {
		strncpy(rx_str, "2", 2);
	} else {
		fprintf(stderr, "SNMP: Bad RX given!\n");
		return;
	}

	if (!snmp_set(ss->write, oid, 'i', rx_str)) {
		fprintf(stderr, "SNMP: Set active RX failed!\n");
	}
}

/**
 * Free resources allocated for the SNMP library
 */
void snmp_free(struct snmp_sessions *ss)
{
	free(ss->read_local.peername);
	free(ss->read_local.community);
	free(ss->write_local.peername);
	free(ss->write_local.community);
	snmp_close_sessions();
}
