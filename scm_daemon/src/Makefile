all:
	gcc -g \
	--std=gnu99 \
	-lpthread \
	-o ../scm_daemon \
	-levent -lm \
	-I. $(shell net-snmp-config --cflags) \
	$(shell pkg-config --cflags --libs libmongoc-1.0) \
	scm_daemon.c \
	dblib.c \
	netlib.c \
	snmplib.c \
	watchdog.c \
	handler_mc.c \
	handler_sdd.c \
	esno_monitor.c \
	handler_signals.c \
	net_segments.c \
	$(shell net-snmp-config --libs)
