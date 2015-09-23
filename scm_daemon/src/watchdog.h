#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "common.h"

// The carry for the LibEvent callback
struct ev_carry_watchdog {
	mongoc_collection_t *dbc;
};

void cb_watchdog(evutil_socket_t fd, short events, void *carry);

#endif // WATCHDOG_H
