#!/bin/bash

sudo mount /dev/sda1 /home/pi/Pictures -o dmask=000,fmask=111
mkdir /home/pi/Pictures/foreground_photos
mkdir /home/pi/Pictures/preview_photos
while true ;do
TIMESTAMP=`date +%s`
RANDOM_FILE=`ls /home/pi/Pictures/background_photos | sort -R | tail -n 1`
RANDOM_FILE_NAME=`echo "${RANDOM_FILE}" | cut -d'.' -f1`

OUTPUT_FILE=${RANDOM_FILE_NAME}
OUTPUT_FILE+="_"
OUTPUT_FILE+=${TIMESTAMP}
OUTPUT_FILE+=".jpg"

PREVIEW_FILE=${RANDOM_FILE_NAME}
PREVIEW_FILE+="_"
PREVIEW_FILE+=${TIMESTAMP}
PREVIEW_FILE+=".jpg"

#echo ${RANDOM_FILE}
#echo ${RANDOM_FILE_NAME}
#echo ${OUTPUT_FILE}

./pi_photobooth /home/pi/Pictures/background_photos/${RANDOM_FILE} /home/pi/Pictures/preview_photos/${PREVIEW_FILE}
raspistill -ss 15000 -t 500 -o /home/pi/Pictures/foreground_photos/${OUTPUT_FILE}
#sudo timeout 2 fbi -T 2 /home/pi/Pictures/preview_photos/${PREVIEW_FILE}
sync
done
