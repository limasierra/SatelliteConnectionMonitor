#!/bin/bash

rx=$1
ns=$2
flags=$3

echo Hello world!
echo Alarm script executed for $ns on $rx .

if [[ $(( ($flags >> 0) & 0x1 )) -eq 1 ]]
then
	echo Validity flag is set
fi

if [[ $(( ($flags >> 1) & 0x1 )) -eq 1 ]]
then
	echo EsNo threshold flag is set
fi
