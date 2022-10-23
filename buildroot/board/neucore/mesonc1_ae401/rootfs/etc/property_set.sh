#!/bin/sh

#LOG_LEVEL=0--no log output from amadec; LOG_LEVEL=1--output amadec log
export LOG_LEVEL=0
export CURLLOG_LEVEL=0
export DASHLOG_LEVEL=0
#audio formats to decode use arm decoder
export media_arm_audio_decoder=ape,flac,dts,ac3,eac3,wma,wmapro,mp3,aac,aac_latm,vorbis,raac,cook,amr,pcm,adpcm
#third party lib modules to use
export media_libplayer_modules=libcurl_mod.so,libdash_mod.so,libvhls_mod.so

#Graphic enviroment
#wayland
export XDG_RUNTIME_DIR=/run/wayland/
export LD_LIBRARY_PATH=/home/neucore/lib/:/home/neucore/lib/gstreamer-1.0:$LD_LIBRARY_PATH
export PATH=/home/neucore/bin/:$PATH
export GSETTINGS_SCHEMA_DIR=/home/neucore/conf
if [ ! -d "/tmp/dbus" ]; then
   mkdir /tmp/dbus
fi

HOME=/root
export DBUS_SESSION_BUS_ADDRESS=unix:abstract=/tmp/dbus-123456
touch /tmp/apprun
source /tmp/apprun
if [ $x == 1 ]; then
  pidof dbus-daemon 2&>1 > /dev/null
  if [ $? -ne 0 ]; then	
    dbus-daemon --session --print-address --fork --address=unix:abstract=/tmp/dbus-123456
    echo "export DBUS_SESSION_BUS_ADDRESS=unix:abstract=/tmp/dbus-123456" > /tmp/dbus/dbus-addr
    echo "DBUS_SESSION_BUS_ADDRESS='unix:abstract=/tmp/dbus-123456'" > /tmp/dbus/dbus.env
    echo "export HOME=/root" >> /tmp/dbus/dbus-addr
    start-stop-daemon -b -m -S -q -p "/var/run/start.pid" -x /home/neucore/script/sbs_init.sh
  fi
fi

echo "x=1" > /tmp/apprun
echo "export x" >> /tmp/apprun
source /tmp/apprun
source /tmp/dbus/dbus-addr
rm /1 /root/1 -rf
