#!/bin/bash

# get correct fuse values from file
. `dirname $0`/fuses || exit

# get fuses
sudo avrdude -p atmega8 -P usb -c stk500v2 -U hfuse:r:high.txt:h  -U lfuse:r:low.txt:h || exit

CURHIGH=`cat high.txt`
CURLOW=`cat low.txt`

if [ "$CURHIGH" == "$HIGH" ]; then
	echo "High fuses match. "
else
	echo "High fuses MISMATCH: $CURHIGH instead of $HIGH"
fi

if [ "$CURLOW" == "$LOW" ]; then
	echo "Low fuses match."
else
	echo "Low fuses MISMATCH: $CURLOW instead of $LOW"
fi

