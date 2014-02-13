#!/bin/bash

MIN_ARGS=0
DEFAULT_PORT="usb"
DEFAULT_PROGRAMMER="avrisp2"
DEFAULT_DEVICE="m88"
DEFAULT_FILE="./stromplatine.hex"

if [ $# -lt $MIN_ARGS ]
then
  echo "Usage: `basename $0` hexfile [port] [programmer] [avr device]"
  exit
fi

FILE=${1:-$DEFAULT_FILE}
PORT=${2:-$DEFAULT_PORT}
PROGRAMMER=${3:-$DEFAULT_PROGRAMMER}
DEVICE=${4:-$DEFAULT_DEVICE}

MEMORY_FLASH="flash:w:$FILE"
#MEMORY_EEPROM=eeprom:w:$(FILE)
MEMORY_OPERATION=$MEMORY_FLASH

echo "Trying to write $FILE"
echo "  Device: $DEVICE"
echo "  Port: $PORT"
echo "  Programmer: $PROGRAMMER"

bin/setfuses.sh && avrdude -V -U $MEMORY_OPERATION -p $DEVICE -P $PORT -c $PROGRAMMER -v -v
