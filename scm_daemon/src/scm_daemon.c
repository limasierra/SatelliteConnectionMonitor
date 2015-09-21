/**
 * Copyright (C) 2015  Laurent Seiler
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "scm_daemon.h"

/**
 * Hi, dear source code reader!
 * This is the starting point of the application. We initialize
 * LibEvent, the MongoDB connection, the NetSNMP library, parse
 * the config file and then add the events: A SIGINT handler,
 * the handler for UDP messages (i.e. for the SDD messages as well
 * as the MODCOD statistics), and two periodic events, namely
 * the server watchdog (used in the web interface) and the alert
 * system, which checks for long-term signal quality degradation.
 * At last, the connections are closed and allocated resources are freed.
 * Header files are common for all source files: Each file.c includes
 * it's file.h. In the header file, related structs are defined and
 * functions are declared. The headers always include common.h,
 * which contains all the necessary includes for everybody.
 * The source code should follow the Linux kernel coding style, available
 * at <www.kernel.org/doc/Documentation/CodingStyle>. Furthermore,
 * tabs should be used for identation, spaces for alignment.
 */
int main(int argc, char **argv)
{
	char *portnum_sdd = "1236";
	char *portnum_mc = "1238";
	int sockfd_sdd, sockfd_mc;

	// Init libevent
	struct event_base *evbase;
	evbase = event_base_new();

	// Init database connection
	mongoc_client_t *db_client;
	mongoc_collection_t *dbc_sdd;
	mongoc_collection_t *dbc_mc;
	mongoc_collection_t *dbc_sys;
	db_client = db_init();
	dbc_sdd = db_connect(db_client, DB_NAME, COLLECTION_NAME_SDD);
	dbc_mc = db_connect(db_client, DB_NAME, COLLECTION_NAME_MC);
	dbc_sys = db_connect(db_client, DB_NAME, COLLECTION_NAME_SYSTEM);

	// Init SNMP sessions
	struct snmp_sessions snmp_sess;
	snmp_init(&snmp_sess);

	// Configure blades / network segments to use
	struct rx_index rx_idx;
	rx_index_init(&rx_idx, &snmp_sess);

	// Bind handler for SIGINT (Ctrl-C)
	struct event *ev_sigint;
	struct ev_carry_sigint c_sigint;
	c_sigint.evbase = evbase;
	ev_sigint = evsignal_new(evbase, SIGINT, cb_handle_sigint, &c_sigint);
	event_add(ev_sigint, NULL);

	// Initialize UDP sockets
	sockfd_sdd = listen_to_udp(portnum_sdd);
	sockfd_mc = listen_to_udp(portnum_mc);

	// Monitor SDD socket and add to event list
	struct event *ev_sdd;
	struct ev_carry_sdd c_sdd;
	struct timeval ev_timeout_sdd = { .tv_sec = 5, .tv_usec = 0 };
	c_sdd.dbc = dbc_sdd;
	c_sdd.rx_idx = &rx_idx;
	reset_sdd_accu(&c_sdd.accu, RX1, 0);
	ev_sdd = event_new(evbase, sockfd_sdd, EV_READ|EV_PERSIST,
	                   cb_recv_sdd_packet, &c_sdd);
	if (HANDLE_SDD_MESSAGES)
		event_add(ev_sdd, &ev_timeout_sdd);

	// Monitor MODCOD socket and add to event list
	struct event *ev_mc;
	struct ev_carry_mc c_mc;
	struct timeval ev_timeout_mc = { .tv_sec = 180, .tv_usec = 0 };
	c_mc.dbc = dbc_mc;
	init_mc_accu(&c_mc.accu);
	ev_mc = event_new(evbase, sockfd_mc, EV_READ|EV_PERSIST,
	                  cb_recv_mc_packet, &c_mc);
	if (HANDLE_MODCOD_MESSAGES)
		event_add(ev_mc, NULL);

	// Monitor EsNo degradation and trigger alarm
	struct event *ev_mon;
	struct ev_carry_mon c_mon;
	struct timeval ev_timer_mon = { 3600, 0 };  // Every hour
	c_mon.dbc = dbc_sdd;
	mon_state_init(&c_mon.state, &rx_idx);
	ev_mon = event_new(evbase, -1, EV_PERSIST,
	                   cb_esno_degradation_monitor, &c_mon);
	event_add(ev_mon, &ev_timer_mon);

	// Add a watchdog-like timer, used in the web interface
	struct event *ev_watchdog;
	struct ev_carry_watchdog c_watchdog;
	struct timeval ev_timer_watchdog = { 60, 0 };
	c_watchdog.dbc = dbc_sys;
	ev_watchdog = event_new(evbase, -1, EV_PERSIST, cb_watchdog,
	                        &c_watchdog);
	event_add(ev_watchdog, &ev_timer_watchdog);
	cb_watchdog(0, 0, &c_watchdog); // Fire once immediately

	// Start event loop
	event_base_dispatch(evbase);

	// Free resources before exit
	close(sockfd_sdd);
	close(sockfd_mc);
	event_free(ev_sigint);
	event_free(ev_sdd);
	event_free(ev_mc);
	event_free(ev_mon);
	event_free(ev_watchdog);
	event_base_free(evbase);
	mon_state_destroy(&c_mon.state);
	rx_index_free(&rx_idx);
	snmp_free(&snmp_sess);
	db_disconnect(dbc_sdd);
	db_disconnect(dbc_mc);
	db_disconnect(dbc_sys);
	db_free(db_client);
	printf("Bye.\n");

	return EXIT_SUCCESS;
}

