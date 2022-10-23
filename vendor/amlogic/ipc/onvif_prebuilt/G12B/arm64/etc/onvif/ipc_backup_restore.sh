#! /bin/sh
#
# factory create, reset for hard or soft, and backup
#

IPC_CMD=/usr/bin/ipc-property

FACTORY_BACKUP_FILE=/etc/onvif/ipc_factory_cfg.tar
CUSTOM_BACKUP_FILE=/tmp/system_backup.tar
CUSTOM_RESTORE_FILE=/etc/onvif/system_backup.tar

FACTORY_RESET_FLAG=/etc/onvif/ipc_factory_resetflag
CUSTOM_RESET_FLAG=/etc/onvif/ipc_custom_resetflag

MANU_KEY=/onvif/device/manufacturer
MANU_FILE=/etc/onvif/manufacturer

PERSIST_KEY="/onvif/backup/persist_file_list"
PERSIST_LIST=$($IPC_CMD get $PERSIST_KEY | awk '{print $2}')

if [ -z "$PERSIST_LIST" ]; then
  PERSIST_LIST=/etc/onvif/ipc.backup.persist.files.list
fi

case $1 in
  factory)
    if [ ! -f $FACTORY_BACKUP_FILE ]; then
      tar cf $FACTORY_BACKUP_FILE -T $PERSIST_LIST
    fi
    ;;
  softreset)
    manustr=$($IPC_CMD get $MANU_KEY)
    echo ${manustr##* } > $MANU_FILE
    touch $FACTORY_RESET_FLAG
    ;;
  hardreset)
    touch $FACTORY_RESET_FLAG
    ;;
  backup)
    tar cf $CUSTOM_BACKUP_FILE -T $PERSIST_LIST
    ;;
  restore)
    if [ -f $FACTORY_RESET_FLAG ]; then
      # do factory reset
      rm $FACTORY_RESET_FLAG
      if [ -f $FACTORY_BACKUP_FILE ]; then
        tar xf $FACTORY_BACKUP_FILE -C /
      fi
      if [ -f $MANU_FILE ]; then
        manu=$(cat $MANU_FILE)
        sed -i 's/\("manufacturer":\).*,/\1 "'$manu'",/g' /etc/ipc.json
        rm -f $MANU_FILE
      fi
    elif [ -f $CUSTOM_RESET_FLAG ]; then
      # do custom reset
      rm -f $CUSTOM_RESET_FLAG
      if [ -f $CUSTOM_RESTORE_FILE ]; then
        tar xf $CUSTOM_RESTORE_FILE -C /
      fi
    fi
    ;;
  verify)
    if [ ! -f $CUSTOM_BACKUP_FILE ]; then
      exit 1
    fi
    f1=$(mktemp -u)
    f2=$(mktemp -u)
    cat $PERSIST_LIST | sort -n > $f1
    tar tf $CUSTOM_BACKUP_FILE | sort -n | sed 's#^#/#' > $f2
    if [ ! -s $f2 ]; then
      rm -f $f1 $f2 $CUSTOM_BACKUP_FILE
      exit 1
    fi
    grep -Fxvq -f $f1 $f2
    if [ $? -eq 1 ]; then
      echo "match or less"
      cp $CUSTOM_BACKUP_FILE $CUSTOM_RESTORE_FILE
      touch $CUSTOM_RESET_FLAG
      ret=0
    else
      echo "not match"
      ret=1
    fi
    rm -f $f1 $f2 $CUSTOM_BACKUP_FILE
    exit $ret
    ;;
  *)
    echo "command not support"
    ;;
esac
