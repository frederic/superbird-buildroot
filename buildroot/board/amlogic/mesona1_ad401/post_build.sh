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

if [ -f $TARGET_DIR/etc/alsa_bsa.conf ]; then
    echo "device=dmixer_auto" > $TARGET_DIR/etc/alsa_bsa.conf
fi

if [ -f $TARGET_DIR/etc/init.d/S44bluetooth ]; then
    sed -i 's/bt_name=\"amlogic\"/bt_name=\"amlogic-A1\"/g' $TARGET_DIR/etc/init.d/S44bluetooth
fi

if [ -f $TARGET_DIR/usr/bin/AMLogic_VUI_Solution_Model.awb ] && [ ! -L $TARGET_DIR/usr/bin/AMLogic_VUI_Solution_Model.awb ]; then
    pushd $TARGET_DIR/usr/bin/
    SRC_MODEL=""
    #AMLogic_VUI_Solution_Model.awb should be a link
    if cmp -s AMLogic_VUI_Solution_Model.awb AMLogic_VUI_Solution_Model_Gen2_2Mics.awb; then
        SRC_MODEL=AMLogic_VUI_Solution_Model_Gen2_2Mics.awb
    fi
    if cmp -s AMLogic_VUI_Solution_Model.awb AMLogic_VUI_Solution_Model_Gen2_4Mics.awb; then
        SRC_MODEL=AMLogic_VUI_Solution_Model_Gen2_4Mics.awb
    fi
    if cmp -s AMLogic_VUI_Solution_Model.awb AMLogic_VUI_Solution_Model_Gen2_6Mics.awb; then
        SRC_MODEL=AMLogic_VUI_Solution_Model_Gen2_6Mics.awb
    fi
    if [ -n "$SRC_MODEL" ]; then
        rm -f AMLogic_VUI_Solution_Model.awb
        ln -s $SRC_MODEL AMLogic_VUI_Solution_Model.awb
    fi
    popd
fi

if [ -d $TARGET_DIR/lib/debug ]; then
    rm -frv $TARGET_DIR/lib/debug
fi

#Our current libidn is v12, but soundai want to link with v11.
if [ ! -L $TARGET_DIR/usr/lib/libidn.so.11 ]; then
  pushd $TARGET_DIR/usr/lib/
  ln -s libidn.so.12 libidn.so.11
  popd
fi

