#!/bin/sh
PATH=/bin:/sbin:/usr/bin:/usr/sbin
WIFI_FILE=/var/www/cgi-bin/wifi/select.txt
ssid="default_amlogic"
password="default_amlogic"
RTK_WIFI_FLAG="NONE"
driver_list="8723ds"
wifi_chip_id=""
wifi_module_id=""
wifi_driver_name=""
MULTI_WIFI=/usr/bin/multi_wifi_load_driver
RTK_FLAG_FILE=/etc/wifi/rtk_station_mode

NAME1=wpa_supplicant
DAEMON1=/usr/sbin/$NAME1
PIDFILE1=/var/run/$NAME1.pid

NAME2=hostapd
DAEMON2=/usr/sbin/$NAME2
PIDFILE2=/var/run/$NAME2.pid

NAME3=dnsmasq
DAEMON3=/usr/sbin/$NAME3
PIDFILE3=/var/run/$NAME3.pid

NAME4=dhcpcd
DAEMON4=/usr/sbin/$NAME4
PIDFILE4=/var/run/${NAME4}-wlan0.pid

##################################################################################################
rtk_support()
{
#wifi_power 2 will print "inf=xxx0" if success
inf=`wifi_power 2 | awk -F = '{print $2}'`
if [ "${inf}" = "" ];then
dir="/sys/bus/mmc/devices/sdio0:0001/sdio0:0001:1/"
else
dir="/sys/bus/mmc/devices/${inf}:0001/${inf}:0001:1/"
fi

wifi_device_id=`cat "${dir}/device"`
wifi_vendor_id=`cat "${dir}/vendor"`
#AP6236 & qca9377 & rtk start single mode
if [  ${wifi_vendor_id} = 0x024c -o ${wifi_vendor_id} = 0x0271 ]
then
        RTK_WIFI_FLAG="TRUE"
fi
}

#stop wifi
function stop_wifi_app() {
echo "Stopp prv wpa_supplicant first"
start-stop-daemon -K -o -p $PIDFILE1 2> /dev/null
sleep 1
echo "Stopp prv hostapd first"
start-stop-daemon -K -o -p $PIDFILE2 2> /dev/null
sleep 1
echo "Stopp prv dnsmasq first"
start-stop-daemon -K -o -p $PIDFILE3 2> /dev/null
sleep 1
echo "Stopp prv dhcpcd first"
start-stop-daemon -K -o -p $PIDFILE4 2> /dev/null
sleep 1
${MULTI_WIFI} station 0
}

#uninstall rtk driver
uninstall_rtk_driver()
{
	echo "removing driver if loaded"
	local cnt=1
	driver_num=`echo $driver_list | awk -F " " '{print NF+1}'`
	while [ $cnt -lt $driver_num ]; do
		loaded_driver=`echo $driver_list | awk -F " " '{print $'$cnt'}'`
		lsmod | grep $loaded_driver
		if [ $? -eq 0 ];then
			echo "loaded_driver=$loaded_driver"
			rmmod $loaded_driver
		fi
		cnt=$((cnt + 1))
	done
}

hostapd_conf_pre()
{
    hostapd /etc/hostapd_temp.conf -e /etc/entropy.bin &
    ifconfig $1 192.168.2.1
    DONE=`start-stop-daemon -S -m -p $PIDFILE3  -x $DAEMON3  -- -i$1  --dhcp-option=3,192.168.2.1 --dhcp-range=192.168.2.50,192.168.2.200,12h -p100`
}

init_wifi_env()
{
killall hostapd
killall wpa_supplicant
killall dnsmasq
killall dhcpcd
}

wifi_mode_setup(){
DONE=`ifconfig wlan0 up > /dev/null`
ifconfig wlan0 &> /dev/null
if [ $? -eq 0 ]; then
	if [[ "$1" == "both" ]]
	then
		DONE=`start-stop-daemon -S -m -p $PIDFILE1 -b -x $DAEMON1 -- -Dnl80211 -iwlan0 -c/etc/wpa_supplicant.conf`
    	iw wlan0 interface add wlan1 type managed
		hostapd_conf_pre wlan1
	elif [[ "$1" == "ap" ]]
	then
		hostapd_conf_pre wlan0
	elif [[ "$1" == "station" ]]
	then
		DONE=`start-stop-daemon -S -m -p $PIDFILE1 -b -x $DAEMON1 -- -Dnl80211 -iwlan0 -c/etc/wpa_supplicant.conf`
	fi
fi
ifconfig wlan0 &> /dev/null

if [[ "$1" == "both" || "$1" == "station" ]]
then
	check_state 8
fi
}

rtk_start_station()
{
	init_wifi_env
	#insmod wifi driver
    ${MULTI_WIFI} station 1
	wifi_mode_setup station
}

rtk_station_mode()
{
	# stop wifi
	stop_wifi_app
	#rmmod wifi driver
	uninstall_rtk_driver
	#start station mode
	rtk_start_station
}

rtk_ap_mode()
{
	#uninstall rtk driver
	uninstall_rtk_driver
	#load driver
	$MULTI_WIFI ap 1
	#start ap mode
	wifi_mode_setup ap
}

rtk_init()
{
	rtk_support
	if [[ "${RTK_WIFI_FLAG}" == "TRUE" ]]
	then
		rtk_station_mode
	fi
	
}
##################################################################################################

parse_paras()
{
	ssid=`sed -n "1p" $WIFI_FILE`
	password=`sed -n "2p" $WIFI_FILE`
	if [ "`echo $password |wc -L`" -lt "8" ];then
		echo "waring: password lentgh is less than 8, it is not fit for WPA-PSK"
	fi
}

ping_test()
{
	killall udhcpc
        if [ $1 -eq 0 ];then
                lightTest --num=12 --times=0 --speed=300 --time=0 --style=0 --mute_led=1 --listen_led=1
                echo "ping fail!! ip is NULL"
                if [ -f "/etc/bsa/config/wifi_status" ]; then
                        echo 0 > /etc/bsa/config/wifi_status
                fi
		return 0
        fi
	echo "now going to ping router's ip: $1 for 5 seconds"
	ping $1 -w 5
	if [ $? -eq 1 ];then
		lightTest --num=12 --times=0 --speed=300 --time=0 --style=0 --mute_led=1 --listen_led=1
		echo "ping fail!! please check"
		if [ -f "/etc/bsa/config/wifi_status" ]; then
			echo 0 > /etc/bsa/config/wifi_status
		fi
	else
		echo "------------------------------------------------1"
		if [[ "${RTK_WIFI_FLAG}" == "TRUE" ]]
		then
			touch ${RTK_FLAG_FILE}
		fi
		echo "ping successfully"
		wpa_cli save_config
		sync
		lightTest --num=12 --times=0 --speed=150 --time=3 --style=30 --mute_led=1 --listen_led=1
		if [ -f "/etc/bsa/config/wifi_status" ] ;then
			echo 1 > /etc/bsa/config/wifi_status
		fi
		sleep 2
		lightTest --num=12 --times=0 --speed=300 --time=0 --style=0 --mute_led=1 --listen_led=1
	fi
}

check_state()
{
	local cnt=1
	while [ $cnt -lt $1 ]; do
		echo "check_in_loop processing..."
		ret=`wpa_cli status | grep "wpa_state"`
		ret=${ret##*=}
		if [ $ret == "COMPLETED" ]; then
			return 1
		else
			cnt=$((cnt + 1))
			sleep 1
			continue
		fi
    done

	return 0
}



wifi_setup()
{
	rtk_init
	parse_paras
	wpa_cli -iwlan0 remove_network 0
	id=`wpa_cli add_network | grep -v "interface"`
	echo "***************wifi setup paras***************"
	echo "**  id=$id                                  **"
	echo "**  ssid=$ssid                              **"
	echo "**  password=$password                      **"
	echo "**********************************************"
	wpa_cli set_network $id ssid \"$ssid\"
	if [[ x"$password" == x || x"$password" == x"NONE" ]]; then
		wpa_cli set_network $id key_mgmt NONE
	else
		wpa_cli set_network $id psk \"$password\"
	fi
	wpa_cli enable_network $id
	wpa_cli reassociate $id
	check_state 15
	if [ $? -eq 0 ] ;then
		echo "connect fail!!"
		if [[ "${RTK_WIFI_FLAG}" == "TRUE" ]]
		then
			killall wpa_supplicant
			#killall hostapd
			#killall dnsmasq
			rm ${RTK_FLAG_FILE}
			rtk_ap_mode
		fi
	else
		echo "start wpa_supplicant successfully!!"
		ip_addr=`udhcpc -q -n -s /usr/share/udhcpc/default.script -i wlan0 2> /dev/null | grep "adding dns*" | awk '{print $3}'`
	fi
	ping_test $ip_addr $id
}

wifi_setup
