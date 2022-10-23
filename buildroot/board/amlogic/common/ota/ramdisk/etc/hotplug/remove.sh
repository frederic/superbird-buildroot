MOUNTS=$(mount | grep $1 | cut -d' ' -f3)
umount $MOUNTS
rm -rf $MOUNTS