#!/bin/sh
# $(TARGET_DIR) is the first parameter

sed -i 's/ttyS0/ttyAML0/g' $1/etc/inittab
rm -frv $1/etc/init.d/S60input
# /usr/bin/app_* are related with avs
rm -frv $1/usr/bin/app_*
rm -frv $1/etc/avskey
# remove the files related to Wifi
rm -frv $1/etc/wifi
rm -frv $1/etc/init.d/S81spotify
rm -frv $1/etc/init.d/S42wifi

rm -frv $1/etc/init.d/S48avs
rm -frv $1/etc/init.d/S49ntp
rm -frv $1/etc/init.d/S5*

rm -frv $1/etc/init.d/S8*
rm -frv $1/etc/init.d/S7*

# install usb automount in mdev
textexist=$(cat $1/etc/mdev.conf | grep upstream_automount)
# echo "textexist = $textexist"
if [ -z "$textexist" ] ; then
	sed -i '$a# mount USB automatically' $1/etc/mdev.conf
	sed -i '$asd[a-z].*   root:root 660 */etc/upstream_automount' $1/etc/mdev.conf
fi


