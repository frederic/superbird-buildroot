#!/bin/sh

function wac_setwifi()
{
    killall dhcpcd
    network=$(wpa_cli list_network | grep "$1" | grep -o ^[0-9])
    if [ -z "$network" ]; then
        network=$(wpa_cli -i wlan0 add_network)
    fi
    wpa_cli set_network $network ssid  "\"$1\""
    wpa_cli set_network $network psk "\"$2\""
    wpa_cli select_network $network
    wpa_cli save_config
    dhcpcd wlan0 -t 0 -b -d
}

function wac_apstart()
{
    hostapd_cli DISABLE
    hostapd_cli SET vendor_elements $1
    hostapd_cli SET wpa 0
    hostapd_cli ENABLE
}

function wac_apstop()
{
    pkill -SIGHUP hostapd
}

case "$1" in
    setwifi)
        shift
        wac_setwifi "$@"
	;;
    apstart)
        shift
        wac_apstart "$@"
	;;
    apstop)
        shift
        wac_apstop "$@"
	;;
    *)
	;;
esac
