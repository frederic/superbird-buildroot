#!/bin/sh
#/etc/ipc/isp_sensor_detect.sh

propset="/usr/bin/ipc-property set"
propget="/usr/bin/ipc-property get"

# property keys
prop_isp="/ipc/isp"
prop_ts_main_res="/ipc/video/ts/main/resolution"
prop_vr_res="/ipc/video/vr/resolution"

# get resolution settings from property
ts_main_res=$($propget $prop_ts_main_res | awk '{print $2}')
vr_res=$($propget $prop_vr_res | awk '{print $2}')

# read out sensor resolution information
sensor_info_node="/sys/devices/platform/sensor/info"
sensor_name=$(echo name > $sensor_info_node && \
  cat $sensor_info_node 2> /dev/null || \
  echo "unknown")

sensor_resolution=$(echo resolution > $sensor_info_node && \
  cat $sensor_info_node 2> /dev/null || \
  echo "2MP/1920x1080")

res_format=${sensor_resolution%/*}
default_res=${sensor_resolution#*/}

resinfo_dir="/etc/ipc/resolution"
resinfo_file="$resinfo_dir/$res_format"

# get the supported resolution list
function get_resolution_list() {
  local cfg=$1 # config file
  local res=$2 # default resolution
  if [ -f $cfg ]; then
    # check if the sensor resolution is in the config list
    if cat $cfg | grep -q $res; then
      echo $(cat $cfg | tr '\n' ',' | sed 's/,$//')
      return 0
    fi
  fi

  return 1
}

echo "retrieve supported reslutions from $resinfo_file"
resinfo=$(get_resolution_list $resinfo_file $default_res)

if [ ! $? -eq 0 ]; then
  echo "try other config"
  resinfo_file="$resinfo_dir/${res_format}_${sensor_name}"
  echo "retrieve supported reslutions from $resinfo_file"
  resinfo=$(get_resolution_list $resinfo_file $default_res)
  if [ ! $? -eq 0 ]; then
    echo "resolution list not found, use the default resolution"
    resinfo=$default_res
  fi
fi

# save the supported resolutuion list into property
$propset "/ipc/isp/sensor/name" "$sensor_name"
$propset "/ipc/isp/sensor/resolution/format" "$res_format"
$propset "/ipc/isp/sensor/resolution/list" "$resinfo"

# validate the video resolution settings
# main
if echo $resinfo | grep -vq "$ts_main_res"; then
  $propset $prop_ts_main_res "${resinfo%%,*}"
fi
# vr
if echo $resinfo | grep -vq "$vr_res"; then
  $propset $prop_vr_res "${resinfo%%,*}"
fi
