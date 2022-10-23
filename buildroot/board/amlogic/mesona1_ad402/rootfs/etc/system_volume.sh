#!/bin/sh

curvol_file="/etc/current_volume" 

cmd_str=$1
cmd_val=$2

if [ ! -f /tmp/amixer_vol_ctrl ]; then
	amixer_vol_ctrl=`amixer controls | grep "DAC Digital Playback Volume"` > /dev/null
	echo "$amixer_vol_ctrl" > /tmp/amixer_vol_ctrl
else
	amixer_vol_ctrl=$(cat /tmp/amixer_vol_ctrl)
fi

vol_ctrl_id=${amixer_vol_ctrl%%,*}
if [ ! -f /tmp/amixer_vol_ctrl_max ]; then
	volumeMax=`amixer cget "$vol_ctrl_id" | grep "type=" |awk -F, '{print $5}' | awk -F= '{print $2}'`
	echo "$volumeMax" > /tmp/amixer_vol_ctrl_max
else
	volumeMax=$(cat /tmp/amixer_vol_ctrl_max)
fi

if [ ! -f /tmp/amixer_vol_ctrl_min ]; then
	volumeMin=`amixer cget "$vol_ctrl_id" | grep "type=" |awk -F, '{print $4}' | awk -F= '{print $2}'`
	echo "$volumeMin" > /tmp/amixer_vol_ctrl_min
else
	volumeMin=$(cat /tmp/amixer_vol_ctrl_min)
fi

volumeCurrent=`amixer cget "$vol_ctrl_id" | grep ": values=" |awk -F, '{print $2}'`


SetVolume()
{
	local _newVolume=$1
	if [ $_newVolume -lt $volumeMin ];then
		_newVolume=$volumeMin
	fi
	if [ $_newVolume -gt $volumeMax ];then
		_newVolume=$volumeMax
	fi
    if [ $_newVolume -ne $volumeCurrent ];then
        #amixer cset "$vol_ctrl_id" $_newVolume,$_newVolume 2&>1 > /dev/null
        amixer cset "$vol_ctrl_id" $_newVolume,$_newVolume
        echo "$_newVolume,$_newVolume" > $curvol_file
    else
        echo "Skip set same volume to mixer: $_newVolume"
    fi
}


case "$cmd_str" in
    "vol-up")
        newVolume=$((volumeCurrent+1))
		SetVolume $newVolume
        ;;
    "vol-down")
        newVolume=$((volumeCurrent-1))
		SetVolume $newVolume
     	;;
    "set")
		SetVolume $cmd_val
        ;;
    "get")
		echo $volumeCurrent
        ;;
    "min")
		echo $volumeMin
        ;;
    "max")
		echo $volumeMax
        ;;
    "set_percent")
		newVolume=$(( cmd_val*(volumeMax-volumeMin)/100+volumeMin ))
		SetVolume $newVolume
        ;;
    "get_percent")
		newVolume=$(( (volumeCurrent-volumeMin)*100/(volumeMax-volumeMin) ))
		echo $newVolume
        ;;
    *)
        echo "unknown cmd, vol-up|vol-down|set|get|min|max|set_percent|get_percent";;
esac


