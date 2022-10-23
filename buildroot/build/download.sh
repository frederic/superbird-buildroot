#!/bin/bash

ping_method()
{
	#file check
	if [ -e $1/$2 ]
	then
		exit 0
	fi

	# newwork check
	ping openlinux2.amlogic.com -c 1
	if [ $? -ne 0 ]
	then
		exit 0
	fi

	ping_time=3
	openlinux2_value=`ping openlinux2.amlogic.com -c ${ping_time} | grep "time=" | cut -d '=' -f 4 | cut -d ' ' -f 1`
	openlinux_value=`ping openlinux.amlogic.com -c ${ping_time} | grep "time=" | cut -d '=' -f 4 | cut -d ' ' -f 1`
	count=1
	openlinux2=0
	openlinux=0
	while [ ${count} -le ${ping_time} ]
	do
		openlinux2_value1=`echo ${openlinux2_value} | cut -d ' ' -f ${count}`
		openlinux_value1=`echo ${openlinux_value} | cut -d ' ' -f ${count}`
	let openlinux2=${openlinux2_value1%.*}+${openlinux2}
	let openlinux=${openlinux_value1%.*}+${openlinux}
	let count=${count}+1
	done
	count=0

	let openlinux2=${openlinux2}/${ping_time}
	let openlinux=${openlinux}/${ping_time}
	
	if [ ${openlinux2%.*} -gt ${openlinux%.*} ]
	then
		wget -P $1  http://openlinux.amlogic.com:8000/download/GPL_code_release/ThirdParty/$2
	else
		wget -P $1  http://openlinux2.amlogic.com:8000/download/GPL_code_release/ThirdParty/$2
	fi
	exit 0
}

timezone_method()
{
	time_zone=`date +%z`
	if [ "${time_zone}" -ne "+0800" ]
	then
		wget -P $1  http://openlinux2.amlogic.com:8000/download/GPL_code_release/ThirdParty/$2
	else
		wget -P $1  http://openlinux.amlogic.com:8000/download/GPL_code_release/ThirdParty/$2
	fi

	exit 0
}

ping_method $1 $2
#timezone_method $1 $2
