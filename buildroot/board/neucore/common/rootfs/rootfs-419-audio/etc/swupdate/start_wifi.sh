#!/bin/sh

multi_wifi_load_driver station 1
sleep 3
wpa_supplicant -iwlan0 -Dnl80211 -c /etc/wpa_supplicant.conf &
sleep 3
dhcpcd
sleep 3
