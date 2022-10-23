#!/bin/sh
# $(TARGET_DIR) is the first parameter
# This script file to change rsyslogd.conf
# audioservice can output debug log to /var/log/audioservice.log

TARGET_DIR=$1

if [ -f $TARGET_DIR/etc/alsa_bsa.conf ]; then
    echo "device=dmixer_auto" > $TARGET_DIR/etc/alsa_bsa.conf
fi

if [ -f $1/etc/init.d/S44bluetooth ]; then
    sed -i 's/bt_name=\"amlogic\"/bt_name=\"amlogic-A1\"/g' $1/etc/init.d/S44bluetooth
fi

