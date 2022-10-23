#!/system/bin/sh
# TDK auto test script
#

COUNTER=0
MAX=100
SUCCESS="SUCCESS"

if [ x$1 != x ]
then
	MAX=$1
fi

echo "===== TDK auto test START ====="

while [ "$COUNTER" -lt "$MAX" ]; do
	echo "run $COUNTER time"
	tee_xtest
	if [ $? != 0 ]
	then
		SUCCESS="FAIL"
		break
	fi
	COUNTER=$(($COUNTER+1))
	sleep 1
done

echo "===== Total $MAX times ====="
echo "===== Actual $COUNTER times ====="
echo "===== TDK auto test DONE: $SUCCESS ====="
