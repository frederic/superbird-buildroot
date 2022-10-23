#!/bin/sh

CONFIG=/etc/AlexaClientSDKConfig.json
PROCESS=/usr/bin/SampleApp
DISPLAYCARD=/sbin/DisplayCardsD

if cat /proc/device-tree/compatible | grep "amlogic,a1"; then
    A113L_PLF=1
fi

start_avs() {
    local channel=0
    if [ -f $CONFIG ];then
        mkdir -p /data/share/avs/
        cd /usr/bin/
        if [ -n "$A113L_PLF" ]
        then
            echo "get amlogic A1 board"
            /usr/bin/switch_mics_num.sh 2
            let channel=4
        else
            echo "get amlogic A113 board"
            /usr/bin/switch_mics_num.sh 6
            let channel=16
        fi
        ./SampleApp $CONFIG NONE back $channel &
        cd -
    else
        echo "avs start fail: not found config file $CONFIG!!!"
    fi
}

start_speaker_process () {
    /usr/bin/speaker_process &
}

if [ -z "$A113L_PLF" ]; then
    modprobe snd-aloop

    amixer_ctr=`amixer controls | grep "Loopback Enable"` > /dev/null
    loop_id=${amixer_ctr%%,*}
    amixer cset $loop_id 1

    amixer_ctr=`amixer controls | grep "datain_datalb_total"` > /dev/null
    datalb_id=${amixer_ctr%%,*}
    amixer cset $datalb_id 16

    if [ -f $DISPLAYCARD ]; then
        $DISPLAYCARD -r 270 &
    else
        echo "displaycard start fail!"
    fi
fi

if [ -r /etc/last_date ]; then
    saved_date=$(cat /etc/last_date)
    date -s "$saved_date" > /dev/null
    if [ $? = 0 ]; then
        echo "Set with last saved date $saved_date OK"
    fi
fi

while true ; do
    if [ -z "$A113L_PLF" ]; then
        ps -fe|grep speaker_process |grep -v grep 1>/dev/null 2>&1
        if [ $? -ne 0 ]; then
            echo "start speaker_process....."
            start_speaker_process
        fi
        #Just avoid script error incase SBR will delete speaker process
        echo "" > /dev/null
    fi

    ps -fe|grep SampleApp |grep -v grep 1>/dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "start avs....."
        start_avs
    fi
    if [ -f /etc/init.d/avs_mdns.sh ]; then
        # start avs mdns service for avs setup
        /etc/init.d/avs_mdns.sh
    fi
    sleep 10
done
