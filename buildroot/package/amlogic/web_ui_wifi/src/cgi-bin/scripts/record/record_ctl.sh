#!/bin/sh
PATH=/bin:/sbin:/usr/bin:/usr/sbin
FILENAME=$2
RATE=$3
CHANNEL=$4
BITS=$5
case $1 in
	start)
	arecord -Dhw:0,3 -f $BITS -r $RATE -c $CHANNEL ../record/$FILENAME.wav &
	;;
	done)
	killall -9 arecord
	;;
	delete)
	rm ../record/$FILENAME
	;;
	play)
	aplay -D hw:0,2 ../record/$FILENAME
	;;
esac
