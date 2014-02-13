#!/bin/bash

DEFAULT_PROJECT=stromplatine
PROJECT=${1:-$DEFAULT_PROJECT}

DEFAULT_CONFIG=release
CONFIG=${2:-$DEFAULT_CONFIG}

if [ $PROJECT = "stromplatine" ]
then
	echo -e "Compiling stromplatine project\n"
	bin/premake4 --platform=avr gmake
	make -R verbose=1 config=$CONFIG $PROJECT 
else
	echo -e "Compiling $PROJECT project ...\n"
	bin/premake4 gmake
	make -R config=$CONFIG $PROJECT
fi

