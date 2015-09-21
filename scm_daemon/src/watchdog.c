#include "watchdog.h"

/**
 * Callback for the periodic watchdog: A timestamp is updated in the
 * database. This allows the server to check if the daemon is up and running.
 */
void cb_watchdog(evutil_socket_t fd, short events, void *carry)
{
	time_t ts;
	mongoc_collection_t *dbc;

	// Unpack carry
	dbc = ((struct ev_carry_watchdog *)carry)->dbc;

	// Get current time stamp
	ts = time(NULL);

	// Update value in database
	db_update_watchdog(dbc, ts);
}
