#ifndef SNMPLIB_H
#define SNMPLIB_H

#include "common.h"

struct snmp_sessions {
	netsnmp_session read_local;
	netsnmp_session write_local;
	netsnmp_session *read;
	netsnmp_session *write;
};

void snmp_init(struct snmp_sessions *ss);
void snmp_activate_default_profile(struct snmp_sessions *ss, size_t rx);
void snmp_set_active_profile(struct snmp_sessions *ss, size_t rx, unsigned char profile);
void snmp_set_freq(struct snmp_sessions *ss, size_t rx, const char *frequency);
void snmp_set_active_rx(struct snmp_sessions *ss, size_t rx);
void snmp_free(struct snmp_sessions *ss);

#endif // SNMPLIB_H
