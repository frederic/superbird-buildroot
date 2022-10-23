#!/bin/sh
# This script file is used to get default system name.

SYSNAMEFILE=/etc/board_info
default_name=""

# if there is no any parameter, it means get default system name
if [ $# -eq 0 ] ; then
	if [ -f $SYSNAMEFILE ] ; then
		default_name=$(cat $SYSNAMEFILE | grep "default-name=")
		if [ -n "$default_name" ]; then
			default_name=${default_name#default-name=}
		fi
	fi
fi

if [ -n "$default_name" ]; then
	echo $default_name
fi
