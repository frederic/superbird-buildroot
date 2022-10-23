#!/bin/sh

case "$1" in
    start|"")
	if [ ! -d /tmp/avahi-autoipd ]; then
	    rm -rf /tmp/avahi-autoipd
	    mkdir /tmp/avahi-autoipd
	    chown avahi.avahi /tmp/avahi-autoipd
	fi
	;;
    stop) ;;
    netready) ;;
    netup) ;;
    netdown) ;;
    netchange) ;;
    *)
	echo "Usage: S05avahi-setup.sh {start|stop}" >&2
	exit 1
	;;
esac
