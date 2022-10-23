#!/bin/sh
PATH=/bin:/sbin:/usr/bin:/usr/sbin

if [ $1 == "start" ];then
  if [ -f "/proc/inand" ]; then
    swupdate --recovery -l 6 -k /etc/swupdate-public.pem -w "-r /var/www/swupdate/" > /tmp/swupdate.log &
  else
    swupdate --recovery -l 6 -k /etc/swupdate-public.pem -b "0 1 2 3 4" -w "-r /var/www/swupdate/" > /tmp/swupdate.log &
  fi
else
killall swupdate
fi
