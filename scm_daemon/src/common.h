#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <event2/event.h>
#include <mongoc.h>
#include <bson.h>
#include <bcon.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include "scm_daemon.h"
#include "dblib.h"
#include "netlib.h"
#include "snmplib.h"
#include "watchdog.h"
#include "handler_sdd.h"
#include "handler_mc.h"
#include "esno_monitor.h"
#include "handler_signals.h"
#include "net_segments.h"

/* Application-specific settings */
#define SDD_TIME_SLICE 30  // switching interval in seconds
#define TC1_IP_ADDR "192.168.1.50"  // TC1 IP address, for UDP message listening
#define NS_CONFIG_FILE "config.txt" // Parsed to get network segments
#define MON_ALARM_EXE "esno_monitor.sh" // Script to execute for EsNo monitor
#define MON_OBSERVATION_TIME 86400  // Monitor time slice for last average in seconds
#define HANDLE_SDD_MESSAGES 1  // Whether or not SDD (EsNo) messages should be captured
#define HANDLE_MODCOD_MESSAGES 0  // Whether or not the MODCOD stats should be captured

/* Database-specific settings */
#define DB_NAME "tc1"  // Name of database to use
#define COLLECTION_NAME_SDD "sdd"  // Name of collection for SDD stats
#define COLLECTION_NAME_MC "mc"  // Name of collection for MODCOD stats
#define COLLECTION_NAME_SYSTEM "sys"  // Name of collection for internal system stuff

/* SNMP-specific settings */
#define TC1_DEFAULT_PROFILE 0  // 0 or 1
#define SNMP_RX1_CFG1_TUNER ".1.3.6.1.4.1.27928.108.1.1.1.1.1.1"  // Frequency
#define SNMP_RX1_CFG2_TUNER ".1.3.6.1.4.1.27928.108.1.1.1.2.1.1"  // Frequency
#define SNMP_RX2_CFG1_TUNER ".1.3.6.1.4.1.27928.108.1.1.2.1.1.1"  // Frequency
#define SNMP_RX2_CFG2_TUNER ".1.3.6.1.4.1.27928.108.1.1.2.2.1.1"  // Frequency
#define SNMP_RX1_CFG1_MODE ".1.3.6.1.4.1.27928.108.1.1.1.3.1"  // (De-)Activate profile
#define SNMP_RX1_CFG2_MODE ".1.3.6.1.4.1.27928.108.1.1.1.3.2"  // (De-)Activate profile
#define SNMP_RX2_CFG1_MODE ".1.3.6.1.4.1.27928.108.1.1.2.3.1"  // (De-)Activate profile
#define SNMP_RX2_CFG2_MODE ".1.3.6.1.4.1.27928.108.1.1.2.3.2"  // (De-)Activate profile
#define SNMP_RX_MGMT ".1.3.6.1.4.1.27928.108.1.1.3.1"  // Activate RX1 or RX2 (with 1 or 2)
#define SNMP_RX1_STATUS ".1.3.6.1.4.1.27928.108.1.1.1.4.1"  // Tuner (un-)locked
#define SNMP_RX2_STATUS ".1.3.6.1.4.1.27928.108.1.1.2.4.1"  // Tuner (un-)locked

/* Defines for internal use */
#define SDD_BUFSIZ 200
#define MC_BUFSIZ 1000

#endif // COMMON_H
