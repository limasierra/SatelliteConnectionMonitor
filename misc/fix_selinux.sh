#!/bin/bash

cd /var/www/html/
setsebool -P httpd_can_network_connect 1
restorecon -v -R .
chmod -R 0755 *
cd -

