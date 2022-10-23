#!/bin/sh
#
# control the hdcp2.2 start and stop
#

HDCP_MODE=/sys/class/amhdmitx/amhdmitx0/hdcp_mode
FIRMWARE=/etc/firmware/firmware.le

start() {
    # start hdcp2.2
    hdcp_tx22 -f $FIRMWARE &
}
stop() {
        killall hdcp_tx22
}
restart() {
    stop
    start
}

case "$1" in
    start)
        start
	;;
    stop)
        stop
	;;
    restart)
	restart
	;;
    *)
        echo "Usage: $0 {start|stop|restart}"
	exit 1
esac

exit $?
