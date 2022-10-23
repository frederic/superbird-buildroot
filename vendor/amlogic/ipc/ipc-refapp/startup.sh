#!/bin/sh
#
# ipc-refapp
#

DAEMON=ipc-refapp
DAEMON_ARGS=""
DAEMON_STOP_FLAG=/tmp/ipc-refapp.stop
GPIO_DETECT=/etc/ipc/ircut-gpio-detect.sh

start() {
  echo "detect ircut gpio settings..."
  $GPIO_DETECT
  if killall -0 $DAEMON > /dev/null 2>&1; then
    echo "$DAEMON already running"
    return 1
  fi
  rm -f $DAEMON_STOP_FLAG
  echo "Starting $DAEMON..."
  (
  while true; do
    if [ -f $DAEMON_STOP_FLAG ]; then
      rm -f $DAEMON_STOP_FLAG
      break
    fi
    $DAEMON $DAEMON_ARGS
  done
  ) &
}

stop() {
  if ! killall -0 $DAEMON > /dev/null 2>&1; then
    echo "$DAEMON not running"
    return 1
  fi
  echo "Stopping $DAEMON ..."
  touch $DAEMON_STOP_FLAG
  killall $DAEMON
  retry_cnt=0
  while true; do
   if killall -0 $DAEMON > /dev/null 2>&1; then
      let retry_cnt++
      if [ $retry_cnt -gt 5 ]; then
        echo "Force stopping $DAEMON ..."
        killall -9 $DAEMON
      else
        echo "Waiting for $DAEMON exit..."
      fi
      sleep 1
   else
      break
   fi
  done
  echo "$DAEMON stopped"
}
restart() {
  stop
  start
}

case "$1" in
  start)
    start
    ;;
  stop)
    stop
    ;;
  restart|reload)
    restart
    ;;
  *)
    echo "Usage: $0 {start|stop|restart}"
    exit 1
esac

exit $?

