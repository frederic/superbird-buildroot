#!/bin/bash
# $(TARGET_DIR) is the first parameter
# This script file to change rsyslogd.conf
# audioservice can output debug log to /var/log/audioservice.log

set -x
TARGET_DIR=$1
echo "Run post build script to target dir $TARGET_DIR"

if [ -f $TARGET_DIR/etc/alsa_bsa.conf ]; then
    echo "device=music_vol" > $TARGET_DIR/etc/alsa_bsa.conf
fi

if [ -f $TARGET_DIR/etc/init.d/S44bluetooth ]; then
    sed -i 's/bt_name=\"amlogic\"/bt_name=\"amlogic-TM2-SBR\"/g' $TARGET_DIR/etc/init.d/S44bluetooth
fi

if [ -d $TARGET_DIR/lib/debug ]; then
    rm -frv $TARGET_DIR/lib/debug
fi

#echo "Remove unnecessary BSA apps"
#find $TARGET_DIR/usr/bin -name app_* ! -name app_manager -delete

# Change S82airplay2 to fit for this project
echo "change /etc/init.d/S82airplay2 to fit for this project"
if [ -f $1/etc/init.d/S82airplay2 ] ; then
	# sed -i 's/OPTIONS=.*/OPTIONS=\"-D dmixer_avs_auto --ipc-client \/tmp\/homeapp_airplay --mfi-proxy 192.168.11.11 --mfi-port 50001\"/' $1/etc/init.d/S82airplay2
	sed -i 's/OPTIONS=.*/OPTIONS=\"-D dmixer_avs_auto --ipc-client \/tmp\/homeapp_airplay --mfi-device \/dev\/i2c-0 --mfi-address 0x10\"/' $1/etc/init.d/S82airplay2
fi

# One image support 3.1 and 5.1.2
echo "Start to change /etc/init.d/S90audioservice to support output 3.1 and 5.1.2"
if [ -f $1/etc/init.d/S90audioservice ] ; then
	textexist=$(cat $1/etc/init.d/S90audioservice | grep default_audioservice_D621)
	if [ -z "$textexist" ] ; then
		sed -i 's/\/usr\/bin\/audioservice/\ \t&/' $1/etc/init.d/S90audioservice
		sed -i '/default_audioservice.conf/i \\t# touch /etc/D621 for D621(tas5782m) board output audio' $1/etc/init.d/S90audioservice
		sed -i '/default_audioservice.conf/i \\t# else ab311(ad82584f) board output audio' $1/etc/init.d/S90audioservice
		sed -i '/default_audioservice.conf/i \\tif [ -f /etc/D621 ]; then' $1/etc/init.d/S90audioservice
		sed -i '/default_audioservice.conf/i \ \t \ \t/usr/bin/audioservice /etc/default_audioservice_D621.conf&' $1/etc/init.d/S90audioservice
		sed -i '/default_audioservice.conf/i \\telse' $1/etc/init.d/S90audioservice
		sed -i '/default_audioservice.conf/a \\tfi' $1/etc/init.d/S90audioservice
	fi
fi

# Remove some no useful files
rm -frv $1/etc/init.d/S60input

