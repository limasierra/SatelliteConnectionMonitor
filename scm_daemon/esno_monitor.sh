#!/bin/bash

function bit_at_mask
{
        pos=$1
        mask=$2
        test $(( ($mask >> $pos) & 0x1 )) -eq 1 \
        && echo true || echo false
}

# Parse input arguments
rx=$1
ns=$2
flags=$3
flag_invalid=$(bit_at_mask 0 $flags)
flag_esno=$(bit_at_mask 1 $flags)

# Check flags and quit if everything is okay
test $flags -eq 0 && exit 0

# Build alert message
msg="Alert for $ns on $rx.\n"

if $flag_invalid
then
        msg+="Did not receive enough packets, "
        msg+="check if it's configured correctly.\n"
fi

if $flag_esno
then
        msg+="EsNo values fell below threshold. "
        msg+="Check for long-term EsNo degradation in the web "
        msg+="interface!\n"
fi

# Write to system log
logger $msg

