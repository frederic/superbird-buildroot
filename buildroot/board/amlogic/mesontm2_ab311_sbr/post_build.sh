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

# Remove some no useful files
rm -frv $1/etc/init.d/S60input

# Change the /etc/default_audioservice.conf
# The input list should match pure soundbar project
#sed -i 'N;/\n.*\"name\":\t\"AIRPLAY\"/!P;D' $1/etc/default_audioservice.conf
#sed -i '/\"name\":\t\"AIRPLAY\"/,/}],/{//!d}' $1/etc/default_audioservice.conf
#sed -i '/\"name\":\t\"AIRPLAY\"/d' $1/etc/default_audioservice.conf
sed -i 's/\"name\":\t\"GVA\"/\"name\":\t\"BT\"/' $1/etc/default_audioservice.conf
sed -i 's/0x10403/0x10402/' $1/etc/default_audioservice.conf

