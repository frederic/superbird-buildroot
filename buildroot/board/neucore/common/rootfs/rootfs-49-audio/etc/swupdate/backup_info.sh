#!/bin/sh
SWUPDATE_PATH=/data/swupdate

rm -fr $SWUPDATE_PATH
mkdir -p $SWUPDATE_PATH

#Copy swupdate script
cp -a /etc/swupdate/* $SWUPDATE_PATH/

#Copy Wifi setting and drivers
mkdir -p $SWUPDATE_PATH/etc
cp -a /etc/wifi $SWUPDATE_PATH/etc/
cp -a /etc/wpa_supplicant.conf $SWUPDATE_PATH/etc/
cp -a /etc/dhcpcd.conf $SWUPDATE_PATH/etc/

for ko in $(find /lib/modules/ -name dhd.ko)
do
    ko_path=$(dirname $ko)
    mkdir -p $SWUPDATE_PATH/$ko_path/
    cp $ko $SWUPDATE_PATH/$ko_path/
done

