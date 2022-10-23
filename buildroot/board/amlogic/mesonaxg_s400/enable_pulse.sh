#!/bin/sh
# $(TARGET_DIR) is the first parameter

# It's used to change /etc/asound.conf
# set pcm.!default and ctl.!default to pulse directly
echo "Start to change /etc/asound.conf to support pulse plugin"
if [ -f $1/etc/asound.conf ] ; then
	sed -i '/pcm.!default/,/}/d' $1/etc/asound.conf
	sed -i '/ctl.!default/,/}/d' $1/etc/asound.conf
	sed -i '1i\ctl.!default {\n\ttype pulse\n}' $1/etc/asound.conf
	sed -i '1i\pcm.!default {\n\ttype pulse\n}\n' $1/etc/asound.conf
fi

