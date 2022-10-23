#!/bin/sh

window_axis=$(cat /sys/class/graphics/fb0/window_axis)

zoomOutWindow() {
    echo "the key zoomout down"
    display_util zoomout
    echo "window_axis:$window_axis"
}

zoomInWindow() {
    echo "the key zoomin down"
    display_util zoomin
    echo "window_axis:$window_axis"
}

case $1 in
    "zoomOut") zoomOutWindow ;;
    "zoomIn") zoomInWindow ;;
    *) echo "no function to add this case: $1" ;;
esac

exit

