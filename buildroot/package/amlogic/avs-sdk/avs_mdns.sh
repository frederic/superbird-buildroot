#!/bin/sh
wpa_state=$(wpa_cli status | grep wpa_state)
wpa_state=${wpa_state#*=}
ip_address=$(wpa_cli status | grep ip_address)
#echo "ip_address=$ip_address"



# Wifi Status
#DISCONNECTED
#COMPLETED
system_name=""

if [ ! -f "/etc/AlexaClientSDKConfig.json"  ]; then
	#echo "avs is not setuped"
	if [ "$wpa_state" == "COMPLETED" ] && [ -n "$ip_address" ]; then
		#wpa_cli status
		#echo "wifi has been connected!"
		ps -fe|grep "mDNSResponderPosix"|grep -v grep 1>/dev/null 2>&1
		if [ $? -ne 0 ]; then
			echo "AVS mdns service is not exist, enable mDNSResponderPosix"
			sleep 2
			if [ -f /etc/init.d/get_sysname.sh ]; then
				system_name=$(/etc/init.d/get_sysname.sh)
			fi
			if [ -z "$system_name" ] ; then
				system_name="Amlogic"
			fi
			mDNSResponderPosix -n $system_name -t _ssh._tcp -d local. -p 22 -x company=amlogic &
		fi
	fi
fi
