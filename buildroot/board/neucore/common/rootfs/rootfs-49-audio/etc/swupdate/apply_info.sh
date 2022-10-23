#!/bin/sh
SWUPDATE_PATH=$1/swupdate

#Remount recovery as read write
mount -o remount,rw /
mkdir -p /var/db

cp $SWUPDATE_PATH/etc/* /etc  -a
cp $SWUPDATE_PATH/lib/* /lib  -a

