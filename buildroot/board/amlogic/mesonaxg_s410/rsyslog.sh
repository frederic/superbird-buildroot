#!/bin/sh
# $(TARGET_DIR) is the first parameter
# This script file to change rsyslogd.conf
# audioservice can output debug log to /var/log/audioservice.log

# It's used to change /etc/asound.conf
# set pcm.!default and ctl.!default to pulse directly
echo "Start to change /etc/rsyslog.conf to enable audioservice log"
if [ -f $1/etc/rsyslog.conf ] ; then
# audioservice uses syslog local2
# we will set it's log level with info
	textexist=$(cat $1/etc/rsyslog.conf | grep local2)
	# echo "textexist = $textexist"
	if [ -z "$textexist" ] ; then
		sed -i '/local7.*/alocal2.info\t\t\/var\/log\/audioservice.log' \
			$1/etc/rsyslog.conf
	fi

# asr_uartcmd uses syslog local3
# we will set it's log level with *
	textexist=$(cat $1/etc/rsyslog.conf | grep local3)
	# echo "textexist = $textexist"
	if [ -z "$textexist" ] ; then
		sed -i '/local7.*/alocal3.*\t\t\t\/var\/log\/audioservice.log' \
			$1/etc/rsyslog.conf
	fi
fi

