#!/bin/bash
# $(TARGET_DIR) is the first parameter
# This script file to change rsyslogd.conf
# audioservice can output debug log to /var/log/audioservice.log

set -x
TARGET_DIR=$1
echo "Run post build script to target dir $TARGET_DIR"

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

if [ -f $TARGET_DIR/etc/init.d/S59snmpd ]; then
    sed '/^SNMPDOPTS/s@127.0.0.1@@' -i $TARGET_DIR/etc/init.d/S59snmpd
fi

if [ -d $TARGET_DIR/usr/lib/gstreamer-1.0 ] && [ ! -e $TARGET_DIR/usr/lib/gstreamer-1.0/libgstamlwsss.so ]; then
  pushd $TARGET_DIR/usr/lib/gstreamer-1.0/
  if [ -e ../../lib32 ]; then
    ln -s libgstamlwsss.so.a32 libgstamlwsss.so
  else
    ln -s libgstamlwsss.so.a64 libgstamlwsss.so
  fi
  popd
fi

echo "Remove unnecessary BSA apps"
find $TARGET_DIR/usr/bin -name app_* ! -name app_manager -delete

echo "Remove unnecessary decoder driver"
rm -fv $TARGET_DIR/lib/modules/*/kernel/media/amvdec_mpeg4.ko
rm -fv $TARGET_DIR/lib/modules/*/kernel/media/stream_input.ko
rm -fv $TARGET_DIR/lib/modules/*/kernel/media/amvdec_mjpeg.ko
rm -fv $TARGET_DIR/lib/modules/*/kernel/media/amvdec_mmpeg4.ko
rm -fv $TARGET_DIR/lib/modules/*/kernel/media/amvdec_vc1.ko
rm -fv $TARGET_DIR/lib/modules/*/kernel/media/amvdec_h264.ko
rm -fv $TARGET_DIR/lib/modules/*/kernel/media/decoder_common.ko
rm -fv $TARGET_DIR/lib/modules/*/kernel/media/amvdec_mpeg12.ko
rm -fv $TARGET_DIR/lib/modules/*/kernel/media/amvdec_h264mvc.ko
rm -fv $TARGET_DIR/lib/modules/*/kernel/media/amvdec_avs.ko
rm -fv $TARGET_DIR/lib/modules/*/kernel/media/amvdec_mmjpeg.ko
rm -fv $TARGET_DIR/lib/modules/*/kernel/media/amvdec_mh264.ko
rm -fv $TARGET_DIR/lib/modules/*/kernel/media/amvdec_mmpeg12.ko
rm -fv $TARGET_DIR/lib/modules/*/kernel/media/amvdec_vp9.ko
rm -fv $TARGET_DIR/lib/modules/*/kernel/media/amvdec_h265.ko
rm -fv $TARGET_DIR/lib/modules/*/kernel/media/amvdec_real.ko

