#ifndef HANDLER_SIGNALS_H
#define HANDLER_SIGNALS_H

#include "common.h"

// Carry for LibEvent callback function
struct ev_carry_sigint {
	struct event_base *evbase;
};

void cb_handle_sigint(evutil_socket_t sig, short events, void *carry);

#endif // HANDLER_SIGNALS_H
