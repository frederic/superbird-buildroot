#!/bin/sh

powerStateFile="/sys/power/state"
powerResumeFlag="/etc/adckey/powerState"
powerStateChange()
{
        echo "mem" > $powerStateFile
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

	rm /etc/bluetooth/wifi_tool.sh
	ln /var/www/cgi-bin/wifi/wifi_tool.sh  /etc/bluetooth/wifi_tool.sh
	if [ ! -f "/etc/bluetooth/config/wifi_status" ]; then
		touch /etc/bluetooth/wifi_status
		chmod 644 /etc/bluetooth/wifi_status
	fi
	echo 0 > /etc/bluetooth/wifi_status


	hciconfig hci0 > /dev/null
	if [ $? -eq 0 ];then
		bluez_ble_service
	else
		bsa_ble_service
	fi
}

bluez_ble_service()
{
	killall btgatt-server
	bluez_tool.sh reset ble
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
