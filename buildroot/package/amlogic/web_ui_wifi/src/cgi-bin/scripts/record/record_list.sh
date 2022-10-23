#!/bin/sh
PATH=/bin:/sbin:/usr/bin:/usr/sbin

RECORD_DIR=../record

if [ ! -d $RECORD_DIR ];then
	mkdir -p ../record
fi

ls -l $RECORD_DIR | awk '{print $9}' > ./record/data_record_list
sed -i '1d' ./record/data_record_list
echo filename > ./record/pub_name
data=`awk -vFS=, 'NR==FNR{split($0,a,FS);next}{split($0,b,FS);for(i in a){c[i]="\042"a[i]"\042:\042"b[i]"\042"};printf FNR==1?"[":",";$0="{"c[1]"}";printf $0}END{print "]"}' ./record/pub_name ./record/data_record_list`

echo $data > ./record/record_list.txt
