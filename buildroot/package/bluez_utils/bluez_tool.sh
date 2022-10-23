#!/bin/sh
bt=$2

NAME1=bluetoothd
DAEMON1=/usr/sbin/$NAME1
PIDFILE1=/var/run/$NAME1.pid

NAME2=rtk_hciattach
DAEMON2=/usr/sbin/$NAME2
PIDFILE2=/var/run/$NAME2.pid

rtk_bdaddr=/opt/bdaddr
aml_bdaddr=/sys/module/kernel/parameters/btmac

realtek_bt_init()
{
	if [[ x$(cat $aml_bdaddr) != x && x$(cat $aml_bdaddr) != x"(null)" ]];then
		cat $aml_bdaddr > $rtk_bdaddr
	else
		rm -f $rtk_bdaddr
	fi
	modprobe rtk_btuart
	modprobe rtk_btusb
	sleep 1
	start-stop-daemon -S -b -m -p $PIDFILE2 -x $DAEMON2 -- -n -s 115200 /dev/ttyS1 rtk_h5
}

Blue_start()
{
	echo 0 > /sys/class/rfkill/rfkill0/state
	sleep 1
	echo 1 > /sys/class/rfkill/rfkill0/state
	sleep 1

	echo
	echo "|-----start bluez----|"
	if [ $bt = "rtk" ];then
		realtek_bt_init
	else
		modprobe hci_uart
		hciattach -s 115200 /dev/ttyS1 any
	fi
	sleep 1
	local cnt=10
	while [ $cnt -gt 0 ]; do
		hciconfig hci0 2> /dev/null
		if [ $? -eq 1 ];then
			echo "checking hci0 ......."
			sleep 1
			cnt=$((cnt - 1))
		else
			break
		fi
	done

	if [ $cnt -eq 0 ];then
		echo "hcio shows up fail!!!"
		exit 0
	fi

	hciconfig hci0 up
	hciconfig hci0 piscan

	start-stop-daemon -S  -m -p $PIDFILE1 -x $DAEMON1 -- -n &
	sleep 1
	agent 0000 &
	hciconfig hci0 class 0x210404
	echo "|-----bluez is ready----|"

}

Blue_stop()
{
	echo -n "Stopping bluez"
	start-stop-daemon -K -o -p $PIDFILE1
	start-stop-daemon -K -o -p $PIDFILE2
	sleep 2
	rm -f $PIDFILE1
	rm -f $PIDFILE2
	killall hciattach
	rmmod hci_uart
	rmmod rtk_btusb
	sleep 2
	echo 0 > /sys/class/rfkill/rfkill0/state
	echo "


|-----bluez is shutdown-----|"
}

case "$1" in
	start)
		Blue_start &
		;;
	netready|netup|netdown|netchange) ;;
	stop)
		Blue_stop
		;;
	*)
		echo "Usage: $0 {start|stop}"
		exit 1
esac

exit $?

