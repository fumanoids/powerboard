#!/bin/bash

MIN_ARGS=1
DEFAULT_PORT="usb"
DEFAULT_PROGRAMMER="stk500v2"
DEFAULT_DEVICE="atmega8"

if [ $# -lt $MIN_ARGS ]
then
  echo "Usage: `basename $0` outputfile [port] [programmer] [avr device]"
  exit
fi

FILE=$1
PORT=${2:-$DEFAULT_PORT}
PROGRAMMER=${3:-$DEFAULT_PROGRAMMER}
DEVICE=${4:-$DEFAULT_DEVICE}

MEMORY_FLASH="flash:r:$FILE:r"
#MEMORY_EEPROM="eeprom:r:$(FILE):r"
MEMORY_OPERATION=$MEMORY_FLASH

echo "Trying to read flash and write towrite $FILE"
echo "  Device: $DEVICE"
echo "  Port: $PORT"
echo "  Programmer: $PROGRAMMER"

avrdude -U $MEMORY_OPERATION -p $DEVICE -P $PORT -c $PROGRAMMER -v -v
