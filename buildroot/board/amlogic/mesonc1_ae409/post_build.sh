#!/bin/bash
# $(TARGET_DIR) is the first parameter
# This script file to change rsyslogd.conf
# audioservice can output debug log to /var/log/audioservice.log

set -x
TARGET_DIR=$1
echo "Run post build script to target dir $TARGET_DIR"

echo "Create ADB UDC value file $TARGET_DIR/etc/adb_udc_file"
echo ff500000.dwc2_a > $TARGET_DIR/etc/adb_udc_file

echo "Replace ff400000.dwc2_a with ff500000.dwc2_a in $TARGET_DIR/etc/init.d/S89usbgadget"
#USB hw is changed in A1
if [ -f $TARGET_DIR/etc/init.d/S89usbgadget ] ; then
    sed 's@ff400000.dwc2_a@ff500000.dwc2_a@' -i $TARGET_DIR/etc/init.d/S89usbgadget
fi

##Remove sshd due to /dev/random is not ready yet.
#if [ -f $TARGET_DIR/etc/init.d/S50sshd ]; then
#    rm -fr $TARGET_DIR/etc/init.d/S50sshd
#fi

#IPC no need set_display_mode.sh
if [ -f $TARGET_DIR/etc/set_display_mode.sh ]; then
    rm -fr $TARGET_DIR/etc/set_display_mode.sh
fi

#IPC no need S50display
if [ -f $TARGET_DIR/etc/init.d/S50display ]; then
    rm -fr $TARGET_DIR/etc/init.d/S50display
fi

#if [ -f $TARGET_DIR/etc/alsa_bsa.conf ]; then
#    echo "device=dmixer_auto" > $TARGET_DIR/etc/alsa_bsa.conf
#fi

#if [ -f $TARGET_DIR/etc/init.d/S44bluetooth ]; then
#    sed -i 's/bt_name=\"amlogic\"/bt_name=\"amlogic-A1\"/g' $TARGET_DIR/etc/init.d/S44bluetooth
#fi

if [ -d $TARGET_DIR/lib/debug ]; then
    rm -frv $TARGET_DIR/lib/debug
fi

