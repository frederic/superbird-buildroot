#!/bin/sh
PATH=/bin:/sbin:/usr/bin:/usr/sbin
while true
do
	wpa_cli  scan > /dev/null
	wpa_cli scan_result | cut -f 5- > ./wifi/data_wifi_list
	line=`cat ./wifi/data_wifi_list | wc -l`
	if [ $line -gt 2 ];then
		break
	fi
	sleep 0.1
done

sed -i '1,2d' ./wifi/data_wifi_list
echo ssid > ./wifi/pub_name
data=`awk -vFS=, 'NR==FNR{split($0,a,FS);next}{split($0,b,FS);for(i in a){c[i]="\042"a[i]"\042:\042"b[i]"\042"};printf FNR==1?"[":",";$0="{"c[1]"}";printf $0}END{print "]"}' ./wifi/pub_name ./wifi/data_wifi_list`

echo $data > ./wifi/wifi_list.txt
