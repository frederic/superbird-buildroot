#! /bin/sh

START=45

DAEMON=ipc-property-service
DAEMON_PATH=/usr/bin
PID_FILE=/tmp/$DAEMON.pid
CFG_FILE=/etc/ipc/config.json

DAEMON_ARGS="--pid_file $PID_FILE"
DAEMON_ARGS="$DAEMON_ARGS --cfg_file $CFG_FILE"

d_start()
{
    if [ -f $PID_FILE ] && kill -0 $(cat $PID_FILE); then
        echo "$DAEMON already running"
        return 1
    fi

    echo "Starting $DAEMON..."
    $DAEMON_PATH/$DAEMON $DAEMON_ARGS && echo "$DAEMON started"
}

d_stop()
{
    if [ ! -f "$PID_FILE" ] || ! kill -0 $(cat "$PID_FILE"); then
        echo "$DAEMON not running"
        return 1
    fi

    echo "Stopping $DAEMON..."
    kill -15 $(cat $PID_FILE) && rm -f $PID_FILE
    echo "$DAEMON stopped"
}

case "$1" in
      start)
          d_start
          ;;
      stop)
          d_stop
          ;;
      restart)
          echo "Restarting $DAEMON"
          d_stop
          d_start
          ;;
      *)
          echo "Usage: $0 {start|stop|restart}"
          exit 1
          ;;
esac
