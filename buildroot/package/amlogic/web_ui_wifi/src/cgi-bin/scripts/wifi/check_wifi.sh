#!/bin/sh
PATH=/bin:/sbin:/usr/bin:/usr/sbin

wpa_cli status | awk -F"=" '{print $2}' > ./wifi/check_wifi.txt
sed -i '1d' ./wifi/check_wifi.txt
echo info > ./wifi/pub_name
data=`awk -vFS=, 'NR==FNR{split($0,a,FS);next}{split($0,b,FS);for(i in a){c[i]="\042"a[i]"\042:\042"b[i]"\042"};printf FNR==1?"[":",";$0="{"c[1]"}";printf $0}END{print "]"}' ./wifi/pub_name ./wifi/check_wifi.txt`

echo $data > ./wifi/check_wifi.txt
