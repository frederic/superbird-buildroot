#!/bin/sh

powerStateFile="/sys/power/state"
powerResumeFlag="/etc/adckey/powerState"
wake_lockFile="/sys/power/wake_lock"
SB_POWERSCRIPT_FILE="/etc/adckey/soundbar_power.sh"
SB_SUSPENDFLAG="/tmp/sb_suspend"

wait_wake_lock()
{
  #check wake_lock begin
  local cnt=10
  while [ $cnt -gt 0 ]; do
    lock=`cat $wake_lockFile`
    if [ ! $lock ];then
        break
    fi
    sleep 1;
    cnt=$((cnt - 1))
    echo "suspend waiting wake_lock to be released..."
  done
  if [ $cnt -eq 0 ];then
    echo "wait suspend timeout, abort suspend"
    echo "unreleased wake_lock: $lock"
    exit 0
  fi
}

powerStateChange()
{
  if [ -f $SB_POWERSCRIPT_FILE ] ;then
    touch $SB_SUSPENDFLAG
    $SB_POWERSCRIPT_FILE suspend &
  else
    if [ -f $powerResumeFlag ];then
        rm $powerResumeFlag
        return 0;
    fi
    #######suspend#######
    aml_socket aml_musicBox_socket suspend
    wait_wake_lock
    touch $powerResumeFlag
    echo "mem" > $powerStateFile
    ######resume#########
    aml_socket aml_musicBox_socket resume
	fi
}

# as_source_func uses to switch input source
as_source_func()
{
  asplay enable-input $1
}

as_sys_func()
{
  asplay sys-command $1
}

bt_func()
{
  asplay enable-input BT
}

as_soundeffect_func()
{
  asplay soundeffect-set $1
}

echo "adckey_function.sh $1"
if [ -f $SB_POWERSCRIPT_FILE ] ; then
  if [ -f $SB_SUSPENDFLAG ] ; then
    # if use the new suspend mechanism
    # it will check whether /etc/sb_suspend is exist
    # if yes, it means system enter suspend mode and it should not response any keys
    # until resume from suspend.
		echo "suspend stat, ignore the input keys"
    exit
  fi
fi

case $1 in
  "power") powerStateChange ;;
  "volumeup") as_sys_func $1;;
  "volumedown") as_sys_func $1;;
  "source") as_sys_func $1;;
  "mute") as_sys_func $1;;
  "HDMI1") as_source_func $1 ;;
  "HDMI2") as_source_func $1 ;;
  "LINEIN") as_source_func $1 ;;
  "SPDIF") as_source_func $1 ;;
  "HDMIARC") as_source_func $1 ;;
  "bluetooth") bt_func ;;
  "DAP" | "BM" | "DRC" | "POST" | "UPMIX" | "VIRT" | "LEGACY" | "HFILT" | "LOUDNESS" | "VLAMP" | "VMCAL") as_soundeffect_func $1 ;;
  *) echo "no function to add this case: $1" ;;
esac

exit
