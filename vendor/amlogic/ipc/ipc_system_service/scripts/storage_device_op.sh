#!/bin/sh

get_device(){
  if [ $1 == "storage" ]; then
    dev=$(mount | awk '{if($1~/dev\//) print $1}')
  elif [ $1 == "recording" ]; then
    dev=$(mount | awk '{if($1~/dev\//) print $1, $3}')
  else
    echo "failed get device"
    exit 1
  fi
  echo $dev
}

get_info(){
  info=$(df -Th $1 | sed -n "2p")
  fs=$(echo $info | cut -d " " -f2)
  if [ $fs == "fuseblk" ]; then
    fs=$(lsblk --fs -n -p $1 | awk '{print $2}')
  fi
  size=$(echo $info | cut -d " " -f3)
  used=$(echo $info | cut -d " " -f4)
  avaliable=$(echo $info | cut -d " " -f5)
  mp=$(echo $info | cut -d " " -f7)
  echo "$1 $fs $size $used $avaliable $mp"
}

format_dev(){
  if [ -f /tmp/ipc_system.format.lock ]; then
    echo "formatting, waitting"
    exit 1
  else
    touch /tmp/ipc_system.format.lock
    umount -l $1
    if [ ! $? -eq 0 ]; then
      echo "failed umount"
      exit 1
    fi
    echo y | mkfs -t $2 $1 >> /dev/null
    if [ ! $? -eq 0 ]; then
      echo "failed format"
      exit 1
    fi
    mount $1 $3
    if [ ! $? -eq 0 ]; then
      echo "failed mount"
      exit 1
    fi
    info=$(df -Th $1 | sed -n "2p")
    fs=$(echo $info | cut -d " " -f2)
    if [ $2 == $fs ]; then
      echo "finish"
    fi
    rm /tmp/ipc_system.format.lock
  fi
}

if [ $1 == "device" ]; then
  get_device $2;
elif [ $1 == "info" ]; then
  get_info $2;
elif [ $1 == "format" ]; then
  format_dev $2 $3 $4;
else
  echo "unknown parameter"
  exit 1
fi
