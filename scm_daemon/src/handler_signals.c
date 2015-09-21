#include "handler_signals.h"

/**
 * Callback for LibEvent to handle SIGINT (Ctrl+C) event
 */
void cb_handle_sigint(evutil_socket_t sig, short events, void *carry)
{
	printf("SIGINT received, shutting down...\n");

	struct event_base *evbase;

	// Unpack carry
	evbase = ((struct ev_carry_sigint *)carry)->evbase;

	// Exit the main loop of the program
	event_base_loopexit(evbase, NULL);
}

