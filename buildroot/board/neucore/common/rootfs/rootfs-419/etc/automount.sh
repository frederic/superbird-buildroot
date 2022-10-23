#!/bin/sh

cmd=$ACTION
format=$ID_FS_TYPE

#mount_point=$ID_FS_LABEL
#if [ -z $mount_point ];then
#    mount_point=${DEVNAME##*/}
#fi
mount_point=${DEVNAME##*/}

if [ "x$cmd" = "xadd" ];then
    if [  -n $mount_point ];then
        mkdir -p /media/$mount_point
        if [ "x$format" = "xntfs" ];then
            ntfs-3g $DEVNAME /media/$mount_point
        else
            mount $DEVNAME /media/$mount_point
        fi
    fi
else if [ "x$cmd" = "xremove" ];then
    if [ -n $mount_point ];then
        umount -l /media/$mount_point
        ret=$?
        if [ $ret -eq 0 ];then
            rm -r /media/$mount_point
        fi
    fi
fi
fi



