#!/bin/bash
sudo mount /dev/sda1 /home/pi/Pictures -o dmask=000,fmask=111

while true ;do

RANDOM_FILE=`ls /home/pi/Pictures/preview_photos | sort -R | tail -n 1`

sudo timeout 4 fbi -T 2 ${RANDOM_FILE}

done
