#!/bin/sh

if [ $# -lt 1 ]; then
	exit 0;
fi

function get_boot_device() {
	for i in `cat /proc/cmdline`; do
		case "$i" in
			root=*)
				ROOT="${i#root=}"
				;;
		esac
	done
}

function get_update_part() {
	OFFSET=$((${#ROOT}-1))
	CURRENT_PART=${ROOT:$OFFSET:1}
	if [ $CURRENT_PART -eq "1" ]; then
		UPDATE_PART="2";
	else
		UPDATE_PART="1";
	fi
}

function get_update_block_device() {
	UPDATE_ROOT=${ROOT%?}${UPDATE_PART}
}

if [ $1 == "preinst" ]; then
	echo swupdate preinst
fi

if [ $1 == "postinst" ]; then
	echo swupdate postinst
fi
