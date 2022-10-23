#!/bin/sh

powerStateFile="/sys/power/state"
powerResumeFlag="/etc/adckey/powerState"
wake_lockFile="/sys/power/wake_lock"

wait_wake_lock()
{
    #check wake_lock begin
    local cnt=10
    while [ $cnt -gt 0 ]; do
        lock=`cat $wake_lockFile`
        if [ ! $lock ];then
            break
        fi
        sleep 1;
        cnt=$((cnt - 1))
        echo "suspend waiting wake_lock to be released..."
    done
    if [ $cnt -eq 0 ];then
        echo "wait suspend timeout, abort suspend"
        echo "unreleased wake_lock: $lock"
        exit 0
    fi
}

powerStateChange()
{
    #######suspend#######
    aml_socket aml_musicBox_socket suspend
    wait_wake_lock
    echo "mem" > $powerStateFile
    ######resume#########
    aml_socket aml_musicBox_socket resume
}

volumeUpAction()
{
    local volumeMax=`amixer sget "5707_A Master"|grep "Limits:"|awk '{print $4}'`
    local volumeCurrent=`amixer sget "5707_A Master" |grep "Mono:" |awk '{print $2}'`
    if [ $volumeCurrent -le $volumeMax ];then
        let volumeCurrent+=10
        echo "$volumeCurrent"
        if [ $volumeCurrent -ge $volumeMax ];then
            volumeCurrent=$volumeMax
        fi
        amixer sset "5707_A Master" $volumeCurrent
        amixer sset "5707_B Master" $volumeCurrent
    fi
}

volumeDownAction()
{
    local volumeMin=`amixer sget "5707_A Master" |grep "Limits:" |awk '{print $2}'`
    local volumeCurrent=`amixer sget "5707_A Master" |grep "Mono:" |awk '{print $2}'`
    if [ $volumeCurrent -ge $volumeMin ];then
        let volumeCurrent-=10
        if [ $volumeCurrent -lt $volumeMin ];then
            volumeCurrent=$volumeMin
        fi
        amixer sset "5707_A Master" $volumeCurrent
        amixer sset "5707_B Master" $volumeCurrent
    fi
}

wifiSmartConfig()
{
	brcm_smartconfig.sh
}

ble_wifi_setup()
{
	echo "ble config for wifisetup"
	rm /etc/bsa/config/wifi_tool.sh
	ln /var/www/cgi-bin/wifi/wifi_tool.sh  /etc/bsa/config/wifi_tool.sh
	if [ ! -f "/etc/bsa/config/wifi_status" ]; then
		touch /etc/bsa/config/wifi_status
		chmod 644 /etc/bsa/config/wifi_status
	fi
	echo 0 > /etc/bsa/config/wifi_status

	hciconfig hci0 > /dev/null
	if [ $? -eq 0 ];then
		killall btgatt-server
		bluez_tool.sh reset ble rtk
	else
		bsa_ble_service
	fi
}

bsa_ble_service()
{
	local app1_id=`ps | grep "aml_musicBox" | awk '{print $1}'`
	kill -9 $app1_id
	local app2_id=`ps | grep "aml_ble_wifi_setup" | awk '{print $1}'`
	kill -9 $app2_id
	cd /etc/bsa/config
	aml_ble_wifi_setup &
	aml_musicBox  ble_mode &
}

case $1 in
    "power") powerStateChange ;;
    "VolumeUp") volumeUpAction ;;
    "VolumeDown") volumeDownAction ;;
    "longpressWifiConfig") wifiSmartConfig ;;
    "WifiConfig")  ble_wifi_setup ;;
    *) echo "no function to add this case: $1" ;;
esac

exit
