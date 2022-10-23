#!/bin/bash
# $(TARGET_DIR) is the first parameter
# This script file to change rsyslogd.conf
# audioservice can output debug log to /var/log/audioservice.log

set -x
TARGET_DIR=$1
echo "Run post build script to target dir $TARGET_DIR"

#TV no need set_display_mode.sh
if [ -f $TARGET_DIR/etc/set_display_mode.sh ]; then
    rm -fr $TARGET_DIR/etc/set_display_mode.sh
fi

#TV no need S50display
if [ -f $TARGET_DIR/etc/init.d/S50display ]; then
    rm -fr $TARGET_DIR/etc/init.d/S50display
fi

#if [ -f $TARGET_DIR/etc/alsa_bsa.conf ]; then
#    echo "device=dmixer_auto" > $TARGET_DIR/etc/alsa_bsa.conf
#fi

#if [ -f $TARGET_DIR/etc/init.d/S44bluetooth ]; then
#    sed -i 's/bt_name=\"amlogic\"/bt_name=\"amlogic-A1\"/g' $TARGET_DIR/etc/init.d/S44bluetooth
#fi

