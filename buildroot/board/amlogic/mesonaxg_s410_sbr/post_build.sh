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

# Change the ALSA device for BT
if [ -f $1/etc/alsa_bsa.conf ] ; then
	textexist=$(cat $1/etc/alsa_bsa.conf | grep 2to8)
	# echo "textexist = $textexist"
	if [ -z "$textexist" ] ; then
		sed -i 's/dmixer_avs_auto/music_vol/g' $1/etc/alsa_bsa.conf
	fi
fi

# MCU6350 gpio interrupt config
echo "Start to change /etc/init.d/S90audioservice to support mcu6350"
if [ -f $1/etc/init.d/S90audioservice ] ; then
	textexist=$(cat $1/etc/init.d/S90audioservice | grep MCU6350)
	if [ -z "$textexist" ] ; then
		sed -i '/#!\/bin\/sh/a done' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a fi' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a \\techo rising > $NODE_GPIO/edge' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a \\techo in     > $NODE_GPIO/direction' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a \\techo $GPIO_INDEX > /sys/class/gpio/export' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a if [ ! -d "$NODE_GPIO" ]; then' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a NODE_GPIO=/sys/class/gpio/gpio$GPIO_INDEX' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a do' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a for GPIO_INDEX in 415 455' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a # gpio_a18(44) + base(411) = 455' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a # gpio_z4(4) + base(411) = 415' $1/etc/init.d/S90audioservice
		sed -i '/#!\/bin\/sh/a \\n# MCU6350 gpio interrupt config' $1/etc/init.d/S90audioservice
	fi
fi

# Copy related aml_halaudio configure file
# now we use the 5.1.2 8 channels config
if [ -f $1/etc/halaudio/8ch_aml_audio_config.json ] ; then
	cp $1/etc/halaudio/8ch_aml_audio_config.json \
		$1/etc/aml_audio_config.json
fi

# Change BT name to amlogic-sbr froom amlogic
# Remote the logic about check /etc/wifi/ap_name
if [ -f $1/etc/init.d/S44bluetooth ]; then
	sed -i 's/bt_name=\"amlogic\"/bt_name=\"amlogic-sbr\"/g' $1/etc/init.d/S44bluetooth
	sed -i '/while [ $cnt -lt 10 ]/,/done/d' $1/etc/init.d/S44bluetooth

	# use get_system.sh to create bt name
	textexist=$(cat $1/etc/init.d/S44bluetooth | grep get_sysname.sh)
	if [ -z "$textexist" ] ; then
		sed -i 's/MusicBox-//g' $1/etc/init.d/S44bluetooth
		sed -i '/bt_name=\"/iif [ -f \/etc\/init.d\/get_sysname.sh ]; then' $1/etc/init.d/S44bluetooth
		sed -i '/bt_name=\"/i  bt_name=$(\/etc\/init.d\/get_sysname.sh)' $1/etc/init.d/S44bluetooth
		sed -i '/bt_name=\"/ifi' $1/etc/init.d/S44bluetooth
		sed -i '/bt_name=\"/iif [ -z \"$bt_name\" ]; then' $1/etc/init.d/S44bluetooth
		sed -i '/bt_name=\"/a fi' $1/etc/init.d/S44bluetooth
		sed -i '/fw_path=\"\/etc\/bluetooth\"/abt_name=\"\"' $1/etc/init.d/S44bluetooth

	fi

	sed -i 's/-lpm//g' $1/etc/init.d/S44bluetooth
fi

# Remove some no useful files
rm -frv $1/lib/debug
rm -frv $1/etc/init.d/S60input
# /usr/bin/app_* are related with avs
rm -frv $1/usr/bin/app_*
rm -frv $1/etc/avskey
# remove the files related to Wifi
rm -frv $1/etc/init.d/S81spotify


