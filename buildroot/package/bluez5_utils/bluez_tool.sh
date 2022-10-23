#!/bin/sh


if [ $2 ];then
	device=$2
else
	device="rtk"
fi

if [ $3 ];then
	mode=$3
else
	mode="sink"
fi

configure_file="/etc/bluetooth/main.conf"
echo "|--bluez: device = $device mode = $mode--|"

function set_btname()
{
	name_file=/etc/wifi/ap_name
	local cnt=1
	bt_name="amlogic"
	while [ $cnt -lt 10 ]
	do
		if [ -f $name_file ];then
			bt_name=`cat /etc/wifi/ap_name`
			if [ "$bt_name" != "" ];then
				break
			fi
		else
			cnt=$((cnt + 1))
			sleep 1
		fi
	done

	if [ -f $configure_file ];then
		sed -i -e "/Name/aName=MusicBox-$bt_name" -e "/Name/d" $configure_file
	else
		echo "create configure file"
		echo "Name=MusicBox-$bt_name" > $configure_file
		echo "debug=0"
	fi

}

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
	usleep 500000
	rtk_hciattach -n -s 115200 /dev/ttyS1 rtk_h5 &
}

qca_bt_init()
{
	modprobe hci_uart
	usleep 300000
	hciattach -s 115200 /dev/ttyS1 qca 2> /dev/null
}

A2DP_SINK_SERVICE()
{
	echo "|--bluez a2dp-sink/hfp-hf service--|"
	hciconfig hci0 up

	grep -Insr "debug=1" $configure_file > /dev/null
	if [ $? -eq 0 ]; then

	echo "|--bluez service in debug mode--|"
		/usr/libexec/bluetooth/bluetoothd -n -d 2> /etc/bluetooth/bluetoothd.log &
		sleep 1
		bluealsa -p a2dp-sink -p hfp-hf 2> /etc/bluetooth/bluealsa.log &
		usleep 200000
		bluealsa-aplay --profile-a2dp 00:00:00:00:00:00 -d dmixer_avs_auto 2> /etc/bluetooth/bluealsa-aplay.log &
	else
		/usr/libexec/bluetooth/bluetoothd -n &
		sleep 1
		bluealsa -p a2dp-sink -p hfp-hf 2> /dev/null &
		usleep 200000
		bluealsa-aplay --profile-a2dp 00:00:00:00:00:00 -d dmixer_avs_auto 2> /dev/null &
	fi
	default_agent > /dev/null &
	hfp_ctl &

	hciconfig hci0 class 0x240408
	hciconfig hci0 piscan
	hciconfig hci0 inqparms 18:1024
	hciconfig hci0 pageparms 18:1024


}

A2DP_SOURCE_SERVICE()
{
	echo "|--bluez a2dp-source service--|"
	hciconfig hci0 up

	grep -Insr "debug=1" $configure_file > /dev/null
	if [ $? -eq 0 ]; then

	echo "|--bluez service in debug mode--|"
		/usr/libexec/bluetooth/bluetoothd -n -d 2> /etc/bluetooth/bluetoothd.log &
		sleep 1
		bluealsa -p a2dp-source 2> /etc/bluetooth/bluealsa.log &
	else
		/usr/libexec/bluetooth/bluetoothd -n &
		sleep 1
		bluealsa -p a2dp-source 2> /dev/null &
	fi
	default_agent > /dev/null &
}

BLE_SERVICE()
{
	echo "|--bluez ble service--|"
	hciconfig hci0 up
	hciconfig hci0 noscan
	sleep 1
	btgatt-server &
}

service_down()
{
	echo "|--stop bluez service--|"
	killall hfp_ctl
	killall default_agent
	killall bluealsa-aplay
	killall bluealsa
	killall bluetoothd
	killall btgatt-server
        killall hcidump
	hciconfig hci0 down
}

service_up()
{
	if [ $mode = "ble" ];then
		BLE_SERVICE
	elif [ $mode = "source" ];then
		A2DP_SOURCE_SERVICE
	else
		A2DP_SINK_SERVICE
	fi
}

Blue_start()
{
	echo 0 > /sys/class/rfkill/rfkill0/state
	usleep 500000
	echo 1 > /sys/class/rfkill/rfkill0/state

	echo
	echo "|-----start bluez----|"
	set_btname

	if [ $device = "rtk" ];then
		realtek_bt_init
	elif [ $device = "qca" ];then
		qca_bt_init
	else
		modprobe hci_uart
		usleep 300000
		hciattach -s 115200 /dev/ttyS1 any
	fi
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

	grep -Insr "debug=1" $configure_file > /dev/null
	if [ $? -eq 0 ]; then
		hcidump -w /etc/bluetooth/btsnoop.cfa &
	fi

	service_up

	echo "|-----bluez is ready----|"

}

Blue_stop()
{
	echo -n "Stopping bluez"
	service_down
	killall rtk_hciattach
	killall hciattach
	rmmod hci_uart
	rmmod rtk_btusb
	sleep 2
	echo 0 > /sys/class/rfkill/rfkill0/state
	echo
	echo "|-----bluez is shutdown-----|"
}

case "$1" in
	start)
		Blue_start &
		;;
	restart)
		Blue_stop
		Blue_start &
		;;
	up)
		service_up
		;;
	down)
		service_down
		;;
	reset)
		service_down
		service_up
		;;
	stop)
		Blue_stop
		;;
	*)
		echo "Usage: $0 {start|stop}"
		exit 1
esac

exit $?

