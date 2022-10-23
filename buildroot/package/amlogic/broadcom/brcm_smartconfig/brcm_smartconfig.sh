#!/bin/sh
ssid=" "
password=" "
encrypt="psk"
log_file="/tmp/brcm_smartconfig.txt"

NAME1=wpa_supplicant
DAEMON1=/usr/sbin/$NAME1
PIDFILE1=/var/run/$NAME1.pid

NAME3=dnsmasq
DAEMON3=/usr/sbin/$NAME3
PIDFILE3=/var/run/$NAME3.pid

MULTI_WIFI=/usr/bin/multi_wifi_load_driver

alias check_wlan="ifconfig wlan0 2> /dev/null"
alias check_wlan_up="ifconfig | grep wlan0 > /dev/null"
alias check_wpa="wpa_cli ping 2> /dev/null | grep PONG"
alias check_ap_connect="wpa_cli status 2> /dev/null | grep state=COMPLETED"
alias check_smartconfig="cat $log_file | grep ssid > /dev/null"
alias check_dhcpcd="dhcpcd -4 -U wlan0 | grep ip_address "

function check_in_loop() {
	local cnt=1
	while [ $cnt -lt $1 ]; do
		echo "check_in_loop processing..."
		case "$2" in
			check_wlan)
			check_wlan
			;;
			check_wlan_up)
			check_wlan_up
			;;
			check_wpa)
			check_wpa
			;;
			check_ap)
			wpa_cli scan
			sleep 2
			wpa_cli scan_result | grep $3
			;;
			check_ap_connect)
			check_ap_connect
			;;
			check_dhcpcd)
			check_dhcpcd
			;;
			check_smartconfig)
			echo "smartconfig searching ssid & password....."
			brcm_smartconfig > $log_file
			check_smartconfig
			;;
		esac
		if [ $? -eq 0 ];then
			return
		else
			cnt=$((cnt + 1))
			sleep 1
			continue
		fi
	done
	echo "|-------broadcom smartconfig fail!!---------|"
	exit
}

function load_driver() {
	# check whether driver is loaded
	echo "|-------loading_driver---------|"
	lsmod | grep dhd > /dev/null
	if [ $? -eq 0 ];then
		rmmod dhd
		sleep 1
	fi

	$MULTI_WIFI station 1

	check_in_loop 15 check_wlan
	ifconfig wlan0 up
	check_in_loop 15 check_wlan_up
}

function start_smartconfig() {
	echo "|-------start smartconfig---------|"
	#write to file for record
	check_in_loop 5 check_smartconfig
	#read from file
	while read line ; do
		key=`echo $line | awk -F ": " '{print $1}'`
		val=`echo $line | awk -F ": " '{print $2}'`
		case "$key" in
			ssid)
			ssid=$val
			;;
			password)
			password=$val
			;;
		esac
	done < $log_file
	echo "|------ssid=$ssid password=$password-----------------|"
}

function start_wlan1() {
	iw wlan0 interface add wlan1 type managed
	hostapd /etc/hostapd_temp.conf -e /etc/entropy.bin &
	ifconfig wlan1 192.168.2.1
	start-stop-daemon -S -m -p $PIDFILE3  -x $DAEMON3  -- -iwlan1  --dhcp-option=3,192.168.2.1 --dhcp-range=192.168.2.50,192.168.2.200,12h -p100
}

function start_wpa() {
	ps | grep -v grep | grep wpa_supplicant
	if [ $? -eq 0 ];then
		start_wlan1
		return
	fi
	echo "|---starting wpa_supplicant---|"
	start-stop-daemon -S -m -p $PIDFILE1 -b -x $DAEMON1 -- -Dnl80211 -iwlan0 -c/etc/wpa_supplicant.conf
	start_wlan1
	check_in_loop 15 check_wpa
}

function search_ap() {
	echo "|-------searching target AP---------|"
	check_in_loop 10 check_ap $ssid
	ap_info=`wpa_cli scan_result | grep $ssid`

	echo $ap_info | grep PSK > /dev/null
	if [ $? -eq 0 ];then
	echo "psk"
		encrypt=psk
		return
	fi

	echo $ap_info | grep WEP > /dev/null
	if [ $? -eq 0 ];then
	echo "wep"
		encrypt=wep
	else
	echo "open"
		encrypt=open
	fi
}

function connect_ap() {
	echo "|-------connecting target AP---------|"
	id=`wpa_cli list_network | grep ${ssid} | awk -F " " '{print $1}'`
	if [ -n "$id" ];then
		echo "delete same network"
		wpa_cli remove_network $id
	fi

	id=`wpa_cli add_network | grep -v "interface"`
	wpa_cli set_network $id ssid \"${ssid}\" > /dev/null

	if [ "$encrypt" = "open" ]; then
		wpa_cli set_network $id key_mgmt NONE
	elif [ "$encrypt" = "wep" ]; then
		wpa_cli set_network $id key_mgmt NONE
		wpa_cli set_network $id auth_alg OPEN SHARED
		wpa_cli set_network $id wep_key0 \"${password}\"
	else
		wpa_cli set_network $id psk \"${password}\" > /dev/null
	fi
	wpa_cli select_network $id  > /dev/null
	wpa_cli enable_network $id  > /dev/null

	check_in_loop 30 check_ap_connect

	############start dhcp#######################
	echo "starting wifi dhcp..."
	dhcpcd -k wlan0
	if [ $debug -eq 1 ];then
		dhcpcd wlan0
	else
		dhcpcd wlan0 > /dev/null
	fi
	check_in_loop 30 check_dhcpcd
	echo "ap connected!!"
}

function ping_test() {
	echo "|-------ping ap test---------|"
	router_ip=`dhcpcd -4 -U $1 2> /dev/null | grep routers | awk -F "=" '{print $2}' | sed "s/'//g"`

	ping $router_ip -w $2

	if [ $? -eq 1 ];then
		echo "ping fail!! please check"
		exit
	else
		echo "ping successfully"
	fi
}

function main() {
	echo
	echo "|-------broadcom smartconfig start---------|"
	killall hostapd
	killall dnsmasq
	load_driver
	start_smartconfig
	start_wpa
	search_ap
	connect_ap
	ping_test wlan0 4
	#save config
	wpa_cli save
	echo "|-------broadcom smartconfig end---------|"
}


main $1 $2 $3
