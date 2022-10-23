#!/bin/sh

if [ $# -lt 1 ]; then
	exit 0;
fi

if [ $1 == "preinst" ]; then
	echo -----------swupdate update.sh preinst---------------------
	#get system ubi number
	system_mtd_number=$(cat /proc/mtd | grep  -E "system" | awk -F : '{print $1}' | grep -o '[0-9]\+')

	mkdir /tmp/rootfs
	#mount system partition
	if [ ${system_mtd_number} = ""]; then
        echo "can not get system mtd number, maybe emmc device......"
        mount -t ext4 /dev/system /tmp/rootfs
    else
        echo "get system mtd number, now mount system partition......"
	    ubidetach  -m ${system_mtd_number}
	    ubiattach /dev/ubi_ctrl -m ${system_mtd_number}
	    mount -t ubifs /dev/ubi1_0 /tmp/rootfs
    fi

