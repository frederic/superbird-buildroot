#!/bin/sh

if [ $1 == "reboot" ]; then
  reboot
elif [ $1 == "factory_reset" ]; then
  uenv set factory-reset 1
  reboot
else
  echo "unknown parameter"
  exit 1
fi
