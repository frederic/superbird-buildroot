if [ -n "$1" ]; then
  if [ -b /dev/$1 ]; then
    if [ ! -d /media ]; then
      mkdir -p /media
    fi
      mount /dev/$1 /media
    if [ $? -ne 0 ]; then
      rm -rf /media
    fi
  fi
fi