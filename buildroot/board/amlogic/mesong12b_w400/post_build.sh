#!/bin/bash
# $(TARGET_DIR) is the first parameter
# This script file to change rsyslogd.conf
# audioservice can output debug log to /var/log/audioservice.log

set -x
TARGET_DIR=$1
echo "Run post build script to target dir $TARGET_DIR"

if [ -d $TARGET_DIR/usr/lib/gstreamer-1.0 ] && [ ! -e $TARGET_DIR/usr/lib/gstreamer-1.0/libgstamlwsss.so ]; then
  pushd $TARGET_DIR/usr/lib/gstreamer-1.0/
  if [ -e ../../lib32 ]; then
    ln -s libgstamlwsss.so.a32 libgstamlwsss.so
  else
    ln -s libgstamlwsss.so.a64 libgstamlwsss.so
  fi
  popd
fi
