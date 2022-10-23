#!/bin/sh
PATH=/bin:/sbin:/usr/bin:/usr/sbin

SPOTIFY_FILE=./spotify/select.txt

uname=`sed -n "1p" $SPOTIFY_FILE`
pwd=`sed -n "2p" $SPOTIFY_FILE`
dname=`sed -n "3p" $SPOTIFY_FILE`

echo $uname > ./spotify/spotify.conf
echo $pwd >> ./spotify/spotify.conf
echo $dname >> ./spotify/spotify.conf
echo infos > ./spotify/pub_name

info_data=`awk -vFS=, 'NR==FNR{split($0,a,FS);next}{split($0,b,FS);for(i in a){c[i]="\042"a[i]"\042:\042"b[i]"\042"};printf FNR==1?"[":",";$0="{"c[1]"}";printf $0}END{print "]"}' ./spotify/pub_name ./spotify/spotify.conf`
echo $info_data > ./spotify/spotify_info.txt

librespot -u "$uname" -p "$pwd" -n "$dname" -c /mnt
