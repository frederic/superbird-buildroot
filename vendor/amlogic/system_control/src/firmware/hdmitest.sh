#!/bin/sh

# This shell is used for testing HDMI/HDCP
# Example:
#    Test HDCP1.4 under 1080p60hz Y444 8BIT
#    hdcp.workflow.sh 1080p60hz 444,8bit 1
#    Please notice that 420 mode is valid for 2160p50/60hz
#    and 2160p50/60hz 444/rgb mode only has 8bit deepcolor mode

if [ $# -le 0 ]; then
	echo "Usage: hdmitest.sh OUTPUTMODE ATTR HDCPMODE"
	echo "       OUTPUTMODE can be 480p60hz, 720p50hz, 1080p60hz, etc"
	echo "       ATTR can be 444,8bit, 420,10bit, etc"
	echo "       HDCPMODE only can be 1, 2, or 0"
	exit 0
fi

outputmode=$1
attrmode=$2
hdcpmode=$3

res="0 0 1919 1079"

if [ $hdcpmode -eq 1 ] || [ $hdcpmode -eq 2 ]; then
	echo "Test $outputmode with hdcp $hdcpmode"
else
	echo "Test $outputmode without hdcp"
fi
if [ $# -eq 3 ]; then
	echo "Test time $2 seconds, press Ctrl + C to cancel"
fi

# CONSTANT SYSFS NODE
AVMUTE=/sys/class/amhdmitx/amhdmitx0/avmute
HDCP_CTRL=/sys/class/amhdmitx/amhdmitx0/hdcp_ctrl
ATTR_MODE=/sys/class/amhdmitx/amhdmitx0/attr
HDCP_MODE=/sys/class/amhdmitx/amhdmitx0/hdcp_mode
AUTH_RST=/sys/module/hdmitx20/parameters/hdmi_authenticated
PHY=/sys/class/amhdmitx/amhdmitx0/phy
DISP_MODE=/sys/class/display/mode
WIND_AXIS=/sys/class/graphics/fb0/window_axis
FREE_SCALE=/sys/class/graphics/fb0/free_scale
FIRMWARE=/etc/firmware/firmware.le
# check Auth Result
hdcp_auth_result () {
	echo 'hdcp' `cat $HDCP_MODE` 'AuthRst' `cat $AUTH_RST`
	return
}

# start HDCP Auth
hdcp_start () {
	# stop hdcp before start hdcp
	hdcp_stop
	if [ $hdcpmode -eq 1 ]; then
		echo 1 > $HDCP_MODE
		sleep 1
		echo -1 > $AVMUTE
	fi
	if [ $hdcpmode -eq 2 ]; then
		echo 2 > $HDCP_MODE
		sleep 0.01
		hdcp_tx22 -f $FIRMWARE &
		sleep 2
		echo -1 > $AVMUTE
	fi
	sleep 1
}

# stop HDCP Auth
hdcp_stop () {
	pid=`ps|awk '/hdcp_tx22/ && !/awk hdcp_tx22/'|awk '{print $1}'`
	if [ ! -z $pid ]; then
	   kill $pid
	fi
	sleep 0.05
	echo stop14 > $HDCP_CTRL
	echo stop22 > $HDCP_CTRL
}

hdmi_set_outputmode() {
	echo 1 > $AVMUTE
	sleep 0.05
	echo 0 > $PHY
	echo null > $DISP_MODE
	echo $attrmode > $ATTR_MODE
	echo $outputmode > $DISP_MODE
	echo $res > $WIND_AXIS
	echo 0x10001 > $FREE_SCALE
	# If there is no any HDCP, then clear AVMUTE directly
	if [ $hdcpmode -eq 0 ]; then
		echo -1 > $AVMUTE
	fi
	return
}

if [ "$outputmode" == "480p60hz" ]; then
	res="0 0 719 479"
elif [ "$outputmode" == "576p50hz" ] ; then
	res="0 0 719 479"
elif [ "$outputmode" == "720p50hz" ] || [ "$outputmode" == "720p60hz" ] ; then
	res="0 0 1279 719"
elif [ "$outputmode" == "1080p50hz" ] || [ "$outputmode" == "1080p60hz" ] || [ "$outputmode" == "1080p30hz" ] ; then
	res="0 0 1919 1079"
elif [ "$outputmode" == "2160p30hz" ] || [ "$outputmode" == "2160p25hz" ] || [ "$outputmode" == "2160p24hz" ] ; then
	res="0 0 3839 2159"
elif [ "$outputmode" == "2160p50hz420" ] || [ "$outputmode" == "2160p60hz420" ] ; then
	res="0 0 3839 2159"
else
	res="0 0 1919 1079"
fi

hdmi_set_outputmode
if [ $hdcpmode -ne 0 ]; then
	hdcp_start
	hdcp_auth_result
fi

