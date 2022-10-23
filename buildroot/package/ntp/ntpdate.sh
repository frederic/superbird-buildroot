#! /bin/sh
#
# System-V init script for the openntp daemon
#

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
DESC="network time protocol daemon"
NAME=ntpd
DAEMON=/usr/sbin/$NAME
NTPDATE_BIN=/usr/bin/ntpdate

# Gracefully exit if the package has been removed.
test -x $DAEMON || exit 0

# Read config file if it is present.
if [ -r /etc/default/$NAME ]; then
    . /etc/default/$NAME
fi

if [ -x $NTPDATE_BIN ] ; then
    while true ; do
        echo -n "Getting initial time via ntp"
        $NTPDATE_BIN -v -b -t 5 $NTPDATE_OPTS $NTPSERVERS > /dev/null 2>&1
        if [ $? = 0 ]; then
            echo "ntpdate OK"
            /bin/date +%Y-%m-%d > /etc/last_date
            #If the platform have RTC, we will write back to RTC HW
            if [ -e /dev/rtc ] || [ -e /dev/misc/rtc ]; then
                hwclock -w
            fi
            break;
        else
            echo "ntpdate FAIL"
            killall -9 ntpd > /dev/null 2>&1
            sleep 1
        fi
    done
fi

echo -n "Starting $DESC: $NAME"
start-stop-daemon -S -q -x $DAEMON

exit 0
