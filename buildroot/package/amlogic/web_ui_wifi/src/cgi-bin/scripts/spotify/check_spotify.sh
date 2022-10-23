#!/bin/sh
ps -fe | grep librespot | grep -v grep > /dev/null
if [ $? -eq 0 ]
then
echo "I am OK" > ./spotify/spotify_state.txt
else
echo no > ./spotify/spotify_state.txt
fi
