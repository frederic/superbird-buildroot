#!/bin/sh
# $(TARGET_DIR) is the first parameter
# This script file to change rsyslogd.conf
# audioservice can output debug log to /var/log/audioservice.log

# It's used to change /etc/asound.conf
# set pcm.!default and ctl.!default to pulse directly


# Following is enable audioservice debog log to console 
#echo "Start to change /etc/rsyslog.conf to enable audioservice log"
#if [ -f $1/etc/rsyslog.conf ] ; then
## audioservice uses syslog local2
## we will set it's log level with info
#	textexist=$(cat $1/etc/rsyslog.conf | grep local2)
#	# echo "textexist = $textexist"
#	if [ -z "$textexist" ] ; then
#		sed -i '/local7.*/alocal2.info\t\t\/dev\/ttyS0' \
#			$1/etc/rsyslog.conf
#	fi
#
## asr_uartcmd uses syslog local3
## we will set it's log level with *
##	textexist=$(cat $1/etc/rsyslog.conf | grep local3)
##	# echo "textexist = $textexist"
##	if [ -z "$textexist" ] ; then
##		sed -i '/local7.*/alocal3.*\t\t\t\/var\/log\/audioservice.log' \
##			$1/etc/rsyslog.conf
##	fi
#fi

# For rsr_tcl project
echo "Start to change /etc/init.d/S90audioservice to support rsr_tcl"
if [ -f $1/etc/init.d/S90audioservice ] ; then
	textexist=$(cat $1/etc/init.d/S90audioservice | grep rsr_tcl)
	if [ -z "$textexist" ] ; then
		sed -i '/#!\/bin\/sh/a \\nmodprobe snd-aloop' $1/etc/init.d/S90audioservice

		sed -i "/#!\/bin\/sh/a amixer cset name='Hardware resample enable' 3" $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a \\n# for hardware resample with 48K' $1/etc/init.d/S90audioservice

		sed -i '/#!\/bin\/sh/a done' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a fi' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a \\techo out > $NODE_GPIO/direction' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a \\techo $GPIO_INDEX > /sys/class/gpio/export' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a if [ ! -d "$NODE_GPIO" ]; then' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a NODE_GPIO=/sys/class/gpio/gpio$GPIO_INDEX' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a do' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a for GPIO_INDEX in 415 416' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a \\n# For s410 project' $1/etc/init.d/S90audioservice
	fi
fi

# Remove some no useful files
rm -frv $1/lib/debug
rm -frv $1/etc/init.d/S60input
# /usr/bin/app_* are related with avs
rm -frv $1/usr/bin/app_*
rm -frv $1/etc/avskey
# remove the files related to Wifi
rm -frv $1/etc/init.d/S81spotify
rm -frv $1/etc/init.d/S44bluetooth


