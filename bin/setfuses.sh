#!/bin/bash

# get correct fuse values from file
. `dirname $0`/fuses || exit

# set fuses
avrdude -p atmega88 -P usb -c stk500v2 -U hfuse:w:$HIGH:m -U lfuse:w:$LOW:m
