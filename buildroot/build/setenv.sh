#!/bin/bash

#set -x
#export PS4='+ [$(basename ${BASH_SOURCE})] [${LINENO}] '

LOCAL_DIR=$(pwd)
BUILDROOT_DIR=$LOCAL_DIR/buildroot
BUILD_OUTPUT_DIR=$LOCAL_DIR/output

DEFCONFIG_ARRAY=(
    "==========In Maintaining======================"

    "mesonc1_ae400_a64_release"
    "mesonc1_ae400_a64_neu_release"
    "mesonc1_ae409_skt_a64_release"
    "mesonc1_ae401_a64_release"
    "mesonc1_ae401_a64_neu_release"

    "mesona1_ad401_a32_release"
    "mesona1_ad402_a32_release"
    "mesona1_ad409_skt_a32_release"

    "mesontm2_ab301_32_release"
    "mesontm2_ab301_release"
    "mesontm2_ab311_32_release"
    "mesontm2_ab311_release"
    "mesontm2_ab311_SBR_a6432_release"

    "mesonsm1_ac200_release"
    "mesonsm1_ac200_32_release"
    "mesonsm1_ac200-tdk_a6432_release"
    "mesonsm1_ac213_32_release"
    "mesonsm1_ac214_32_release"

    "mesontl1_x301_32_release"
    "mesontl1_x301_release"

    "mesong12b_skt_release"
    "mesong12b_w400_32_release"
    "mesong12b_w400_release"
    "mesong12b_w400_s922x_release"
    "mesong12b_w411_32_release"
    "mesong12b_w411_release"

    "mesong12a_u200_32_release"
    "mesong12a_u211_32_release"

    "mesongxl_p212_32_kernel49"
    "mesongxl_p212_kernel49"
    "mesongxl_p212_tdk_32_kernel49"
    "mesongxl_p230_32_kernel49"
    "mesongxl_p231_32_kernel49"
    "mesongxl_p241_32_kernel49"

    "mesonaxg_s400_32_emmc"
    "mesonaxg_s400_32_emmc_k5.1_release"
    "mesonaxg_s400_32_release"
    "mesonaxg_s400_sbr_32_release"
    "mesonaxg_s400_sbravs_32_release"
    "mesonaxg_s410_sbr_32_release"
    "mesonaxg_s420_32_release"

    "==========Out Of Maintain==============="

    "mesonaxg_a113d_skt"
    "mesonaxg_a113d_skt_32"
    "mesonaxg_a113x_skt"
    "mesonaxg_a113x_skt_32"
    "mesonaxg_s400_emmc"
    "mesonaxg_s400_gva_32_release"
    "mesonaxg_s400_release"
    "mesonaxg_s420_gva_32_release"
    "mesonaxg_s420_release"
    "mesong12a_skt_32_release"
    "mesong12a_skt_release"
    "mesong12a_u200_release"
    "mesong12a_u200_vccktest_32_release"
    "mesong12a_u200_vccktest_release"
    "mesong12a_u211_release"
    "mesong12a_u212_32_release"
    "mesong12a_u212_release"
    "mesong12b_w400_32_wayland_release"
    "mesong12b_w400_vccktest_32_release"
    "mesong12b_w400_vccktest_release"
    "mesong12b_w400_wayland_release"
    "mesongxl_p230_kernel49"
    "mesongxl_p231_kernel49"
    "mesongxl_p241_kernel49"
    "txlx_t962e_r321_32_release"
    "txlx_t962e_r321_release"
)

DEFCONFIG_ARRAY_LEN=${#DEFCONFIG_ARRAY[@]}

FILTER=$1
if [[ "$FILTER" =~ "_" ]]
then
    FILTER=""
    DEFAULT_TARGET=$1
else
    DEFAULT_TARGET=""
    FILTER=$1
    echo "Enable manually build filter: =$FILTER="
fi

i=0
while [[ $i -lt $DEFCONFIG_ARRAY_LEN ]]
do
	let i++
done

function choose_info()
{
  echo
  echo "You're building on Linux"
  echo "Lunch menu...pick a combo:"
  i=1
  while [[ $i -le $DEFCONFIG_ARRAY_LEN ]]
  do
    if [ -n "$FILTER" ] && [ -z "$(echo ${DEFCONFIG_ARRAY[$i]} | grep "$FILTER")" ]; then
      #the build configuration did contain the specified string, hide it to user
      let i++
      continue
    fi

    if [[ ${DEFCONFIG_ARRAY[$i]} =~ "debug" ]]; then
      echo "$((${i})). ${DEFCONFIG_ARRAY[$i]}"
    elif [[ ${DEFCONFIG_ARRAY[$i]} =~ "release" ]]; then
      echo "$((${i})). ${DEFCONFIG_ARRAY[$i]}"
    elif [[ ${DEFCONFIG_ARRAY[$i]} =~ "====" ]]; then
      echo "${DEFCONFIG_ARRAY[$i]}"
    else
      echo "$((${i})). ${DEFCONFIG_ARRAY[$i]}_release"
    fi
    let i++
  done
  echo
}

function get_index() {
  if [ $# -eq 0 ]; then
    return 0
  fi

  i=1
  while [[ $i -le $DEFCONFIG_ARRAY_LEN ]]
  do
    if [[ "${DEFCONFIG_ARRAY[$i]}" =~ "debug" ]]; then
      if [ $1 = "${DEFCONFIG_ARRAY[$i]}" ]; then
        return ${i}
      fi
    elif [[ "${DEFCONFIG_ARRAY[$i]}" =~ "release" ]]; then
      if [ $1 = "${DEFCONFIG_ARRAY[$i]}" ]; then
        return ${i}
      fi
    else
      if [ $1 = "${DEFCONFIG_ARRAY[$i]}_release" ]; then
        return ${i}
      fi
    fi
    let i++
  done
  return 0
}

function get_target_board_type() {
	TARGET=$1
	RESULT="$(echo $TARGET | cut -d '_' -f 2)"
	echo "$RESULT"
}

function get_build_config() {
    TARGET=$1
    RESULT=${TARGET##*_}
    if [[ $RESULT = "debug" ]]; then
        echo "${DEFCONFIG_ARRAY[$index]}"
    elif [[ $RESULT = "release" ]]; then
        echo "${DEFCONFIG_ARRAY[$index]}"
    else
        echo "${DEFCONFIG_ARRAY[$index]}_release"
    fi
}

function get_target_build_type() {
	TARGET=$1
	TYPE="$(echo $TARGET | cut -d '_' -f 1)"
	if [[ $TYPE =~ "meson8" ]]; then
		echo "32"
		return
	fi

	LENGTH="$(echo $TARGET | awk -F '_' '{print NF}')"
	if [ $LENGTH -le 2 ]; then
		echo "64"
	else
		RESULT="$(echo $TARGET | cut -d '_' -f 3)"
		if [ $RESULT = "32" ]; then
			echo "32"
		elif [ $RESULT = "64" ]; then
			echo "64"
		else
			echo "64"
		fi
	fi
}

function choose_type()
{
	choose_info
	local DEFAULT_NUM DEFAULT_VALUE
	DEFAULT_NUM=-1
	DEFAULT_VALUE="no-default-option"

	export TARGET_BUILD_TYPE=
	local ANSWER
    local USER_DFT_OPT=$1
	while [ -z $TARGET_BUILD_TYPE ]
	do
		echo -n "Which would you like? ["$DEFAULT_NUM"] "
		if [ -z "$USER_DFT_OPT" ]; then
			read ANSWER
		else
			echo $USER_DFT_OPT
			ANSWER=$USER_DFT_OPT
		fi

#		if [ -z "$ANSWER" ]; then
#			ANSWER="$DEFAULT_NUM"
#		fi

		if [ -n "`echo $ANSWER | sed -n '/^[0-9][0-9]*$/p'`" ]; then
			if [ $ANSWER -le $DEFCONFIG_ARRAY_LEN ] && [ $ANSWER -gt 0 ]; then
				index=$((${ANSWER}))
				TARGET_BUILD_CONFIG=`get_build_config ${DEFCONFIG_ARRAY[$index]}`
				TARGET_DIR_NAME="${DEFCONFIG_ARRAY[$index]}"
				TARGET_BUILD_TYPE=`get_target_build_type ${DEFCONFIG_ARRAY[$index]}`
				TARGET_BOARD_TYPE=`get_target_board_type ${DEFCONFIG_ARRAY[$index]}`
			else
				echo
				echo "number not in range. Please try again."
				echo
			fi
		else
			get_index $ANSWER
			ANSWER=$?
			if [ $ANSWER -gt 0 ]; then
				index=$((${ANSWER}))
				TARGET_BUILD_CONFIG=`get_build_config ${DEFCONFIG_ARRAY[$index]}`
				TARGET_DIR_NAME="${DEFCONFIG_ARRAY[$index]}"
				TARGET_BUILD_TYPE=`get_target_build_type ${DEFCONFIG_ARRAY[$index]}`
				TARGET_BOARD_TYPE=`get_target_board_type ${DEFCONFIG_ARRAY[$index]}`
			else
				echo
				echo "I didn't understand your response.  Please try again."
				echo
                USER_DFT_OPT=""
#                choose_info
			fi
		fi
		if [ -n "$USER_DFT_OPT" ]; then
			break
		fi
	done
	export TARGET_OUTPUT_DIR="$BUILD_OUTPUT_DIR/$TARGET_DIR_NAME"
}

function lunch()
{
	mkdir -p $TARGET_OUTPUT_DIR
	if [ -n "$TARGET_BUILD_CONFIG" ]; then
		cd buildroot
		echo "==========================================="
		echo
		echo "#TARGET_BOARD=${TARGET_BOARD_TYPE}"
		echo "#BUILD_TYPE=${TARGET_BUILD_TYPE}"
		echo "#OUTPUT_DIR=output/$TARGET_DIR_NAME"
		echo "#CONFIG=${TARGET_BUILD_CONFIG}_defconfig"
		echo
		echo "==========================================="
		make O="$TARGET_OUTPUT_DIR" "$TARGET_BUILD_CONFIG"_defconfig
	fi
	cd $LOCAL_DIR
}
function function_stuff()
{
    choose_type $@
    lunch
}

function check_ccache()
{
	if [ ! -d $TARGET_OUTPUT_DIR/../ccache  ]; then
		echo "CCACHE will be enabled if exist $TARGET_OUTPUT_DIR/../ccache"
		return 0
	fi

	CFG_FILE=$TARGET_OUTPUT_DIR/.config
	if [ ! -f $CFG_FILE ]; then
		echo "Missing .config in your target folder"
		return 1
	fi
	grep "BR2_CCACHE=y" $CFG_FILE && echo "Already patched" && return 0
	echo "BR2_CCACHE=y" >> $CFG_FILE
	echo "BR2_CCACHE_DIR=\"$TARGET_OUTPUT_DIR/../ccache\"" >> $CFG_FILE
	echo "BR2_CCACHE_INITIAL_SETUP=\"\"" >> $CFG_FILE
	echo "BR2_CCACHE_USE_BASEDIR=y" >> $CFG_FILE
    echo "CCACHE is enabled in $CFG_FILE"
}

function_stuff $DEFAULT_TARGET
check_ccache || true
