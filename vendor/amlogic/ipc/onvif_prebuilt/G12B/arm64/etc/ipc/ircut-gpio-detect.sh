#!/bin/sh
# guess the gpio settings from device-tree

# device-tree
device_tree=/proc/device-tree/ircut
ircut_gpio=$device_tree/ircut-gpio
ircut_sequence=$device_tree/ircut-sequence
ircut_chip=$device_tree/ircut-chip
irled_gpio=$device_tree/ir-led-gpio
irled_sequence=$device_tree/ir-led-sequence
irled_chip=$device_tree/ir-led-chip
ldr_gpio=$device_tree/ldr-gpio
ldr_chip=$device_tree/ldr-chip

# property
ipc_property="/usr/bin/ipc-property"
prop_get="$ipc_property get"
prop_set="$ipc_property set"

prop_daynight="/ipc/day-night-switch/hardware"

# check device-tree
for d in $device_tree $ircut_gpio $ircut_sequence $irled_gpio $irled_sequence $ldr_gpio; do
  if [ ! -e $d ]; then
    echo "$d not exists, abort detection"
    exit 1
  fi
done

retrieve_gpio_info() {
  local node=$1
  local key=$2
  local cell_num=0
  local cell_vals=""
  local prop_index=0

  for h in $(hexdump $node -v -e '/1 "%02X" /1 "%02X" /1 "%02X" /1 "%02X\n"');
  do
    let cell_num++
    cell_vals="$cell_vals $(printf "%d" 0x$h)"
  done
  for i in $(seq 1 3 $cell_num);
  do
    let idx=i+1
    for k in line active-low; do
      local v=$($prop_get $key/gpio/$k/$prop_index | awk -F: '{print $2}' | tr -d ' ')
      local s=$(echo $cell_vals | awk '{print $'$idx'}')
      if [ -z "$v" ]; then
        $prop_set $key/gpio/$k/$prop_index "$s" > /dev/null 2>&1
      else
        if [ ! "$v" = "$s" ]; then
          local n=$(echo $node | sed "s;$device_tree/;;")
          echo "$n/$k/$prop_index is set to '$v', dts value ignored: '$s'"
        fi
      fi
      let idx++
    done
    let prop_index++
  done
}

retrieve_gpio_sequence() {
  local seq_node=$1
  local key=$2
  local seq=$(cat $seq_node | tr '\000' '\n')
  for k in init enable disable; do
    local v=$($prop_get $key/sequence/$k | awk -F: '{print $2}' | tr -d ' ')
    local s=$(echo "$seq" | grep $k | awk -F: '{print $2}' | tr -d ' ')
    if [ -z "$v" ]; then
      $prop_set $key/sequence/$k "$s" > /dev/null 2>&1
    else
      if [ ! "$v" = "$s" ]; then
        local n=$(echo $seq_node | sed "s;$device_tree/;;")
        echo "$n/$k is set to '$v', dts value ignored: '$s'"
      fi
    fi
  done
}

retrieve_gpio_chip() {
  local chip_node=$1
  local key=$2
  local val=""
  if [ -e $chip_node ]; then
    h=$(hexdump $chip_node -v -e '/1 "%02X"');
    val="$val$(printf "%d" 0x$h)"
    local v=$($prop_get $key/gpio/chip | awk -F: '{print $2}' | tr -d ' ')
    local s="gpiochip"$val
    if [ -z "$v" ]; then
      $prop_set $key/gpio/chip $s > /dev/null 2>&1
    else
      if [ ! "$v" = "$s" ]; then
        local n=$(echo $chip_node | sed "s;$device_tree/;;")
        echo "$n is set to '$v', dts value ignored: '$s'"
      fi
    fi
  fi
}

retrieve_gpio_info $ircut_gpio $prop_daynight/ircut
retrieve_gpio_sequence $ircut_sequence $prop_daynight/ircut
retrieve_gpio_chip $ircut_chip $prop_daynight/ircut
retrieve_gpio_info $irled_gpio $prop_daynight/irled
retrieve_gpio_sequence $irled_sequence $prop_daynight/irled
retrieve_gpio_chip $irled_chip $prop_daynight/irled
retrieve_gpio_info $ldr_gpio $prop_daynight/ldr
retrieve_gpio_chip $ldr_chip $prop_daynight/ldr
