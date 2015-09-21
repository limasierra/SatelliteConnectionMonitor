Satellite Connection Monitor
============================

This application provides statistics on the satellite connection.
It's goal is to observe long-term signal degradation. It consists
of two main parts: (a) A daemon which logs information to a database, and
(b) a web interface to visually display the gathered statistics.
It is intended to work with an Ayecka TC1, from which it logs Es/N0
values as well as the distribution of MODCODs. The TC1 shall be
connected to a terminal, intended to monitor the satellite connection.
As such, the information is retrieved from incoming traffic. The
project has been realized during a summer job at SES S.A. .


License
-------

```
Copyright (C) 2015  Laurent Seiler

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
```


Requirements
------------

- The application has been developed on an Arch-based GNU/Linux system,
  although it should run on most UNIXes.
- It relies on MongoDB, LibEvent, PHP and NetSNMP. You will need:
  * mongodb
  * php-mongo
  * net-snmp
  * libevent
  * php
  * php-apache
  * php-mongo
  * apache (or equivalent)
- Additionally, it is highly recommended to run the application in a
  Valgrind instance, as the code may still contain bugs. It will not prevent
  them, but you can file a nice bug report :)
  * valgrind
- The web interface is using Twitter Bootstrap and NVD3 for the graphs. The
  required sources are already included.


Installation
------------

- Install the required dependencies
- In `scm_daemon/`:
  * Set the appropriate IP for the TC1 in `src/common.h`, in the line containing
    `#define TC1_IP_ADDR ...`
  * Configure the network segments + frequencies + alarm thresholds in `config.txt`.
  * The script `monitor_alarm.sh` will be executed if an alarm is raised. Adapt
    it to your needs and make sure it is executable (`chmod +x monitor_alarm.sh`).
  * Run the daemon using `./run.sh` to run it in a Valgrind session.
- Copy the files in `web_interface` to a location where Apache can find them, so that
  they are available at `http://localhost`.


Details
-------

- When an alarm is triggered, the `monitor_alarm.sh` script is triggered with the
  following parameters: `./monitor_alarm.sh rx_name ns_name alert_type`. The alert
  type is a bitmask: `0x1` indicates a failed validity check, i.e. not enough
  packets have been received for this network segment to make a representative
  average. `0x2` indicates that the EsNo average has fallen below the threshold
  given in `config.txt`.
- The MODCOD stats are deactivated by default. You can enable them by setting the
  `HANDLE_MODCOD_MESSAGES` define in `common.h` to `1`. A proof-of-concept graph
  can be seen at `localhost/modcods.php` then. Be aware that MODCOD statistics
  are not separated by the switching NS configurations, i.e. they only provide
  a global average for all the configurations in `config.txt`

