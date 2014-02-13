#!/bin/bash

# Install the dependencies 

# determine linux distro
if [ $(which lsb_release 2>/dev/null) ]
then
  distro=$(lsb_release -d | awk {'print $2'} | tr '[:upper:]' '[:lower:]')
  if [ $distro == "linux" ]; then
    distro=$(lsb_release -d | awk {'print $3'} | tr '[:upper:]' '[:lower:]')
  fi
else
  # looking for release files - ugly - should work
  # http://linuxmafia.com/faq/Admin/release-files.html
  distro=$(ls /etc/ | egrep '[-_](release|version)$' | awk -F [-_] {'print $1'} \
           | tr '[:upper:]' '[:lower:]')
fi

case $distro in
  ubuntu|mint)

    # Tested on Ubuntu 10.10

    packages=" \
binutils-avr avr-libc avrdude gcc-avr build-essential \
"

    echo "Installing all dependencies: $packages"

    sudo apt-get install $packages

    ;;

  arch)

    # Tested on Arch Linux Oct 2010
    
    echo
    echo "**WARNING** On x86_84 systems make sure to uncomment multilib in /etc/pacman.conf"
    echo

    packages="binutils-avr avr-libc avrdude gcc-avr"

    echo "Installing all dependencies: $packages"

    sudo pacman -S $packages

    ;;

  *)

    echo "Sorry, your OS is not supported yet"

    ;;
esac

