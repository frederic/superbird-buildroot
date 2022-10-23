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

suspend_handle()
{
	echo "start to suspend"
	# ask BT to suspend
	aml_socket aml_musicBox_socket suspend

	# check audioservice is exist
	as_pid=$(ps -ef|grep asplay.py|grep -v grep)
	as_pid=${playerpid%% *}
	if [ -n as_pid ] ; then
		# as_client should stop all opened decoder
		playerpid=$(ps -ef|grep asplay.py|grep -v grep)
		playerpid=${playerpid%% *}
	  echo "exec kill command pid = $playerpid"
		if [ -n $playerpid ] ; then
			kill -s SIGINT $playerpid
		fi
		# and disable current input source

		asplay.py sys suspend

	fi

	# wait BT to suspend done
  wait_wake_lock

	# notify kernal to enter suspend
	echo "mem" > $powerStateFile

	# notify BT (music_box) to resume
	aml_socket aml_musicBox_socket resume
	# notify audioservice resume
	asplay.sh sys resume

	# remove the suspend flag
	# thus adckey_function.sh can handle input key
	rm $SB_SUSPENDFLAG
}

help()
{
	echo ""
	echo ""
	echo "Please reference following information"
	echo "soundbar_power.sh support command list:"
	echo "	soundbar_power.sh suspend			# suspend system"
}


case $1 in
	"suspend") suspend_handle ;;
	*) echo "input error command $0"
		help $1 ;;
esac
