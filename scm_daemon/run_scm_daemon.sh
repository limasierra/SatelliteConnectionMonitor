#!/bin/bash

while true
do
	valgrind \
		--suppressions=valgrind.suppressions \
		--leak-check=full \
		--show-leak-kinds=all \
		./scm_daemon

	rv=$?

	if [[ $rv -ne 0 ]]
	then
		msg="$(date): SCM Monitor crashed! \n"

		echo -e "$msg"

		echo "Waiting 30 min until server restart..."
		sleep 1800  # 30 min
		echo "Restarting..."

	else
		exit 0
	fi
done
