#!/bin/bash
gpio export 17 in
gpio mode 17 up
sudo mount /dev/sda1 /home/pi/Pictures -o dmask=000,fmask=111
mkdir /home/pi/Pictures/foreground_photos
mkdir /home/pi/Pictures/preview_photos
mogrify -resize 2592x1944^ -gravity center -extent 2592x1944 /home/pi/Pictures/background_photos/*

while true ;do
TIMESTAMP=`date +%s`
RANDOM_FILE=`ls /home/pi/Pictures/background_photos | sort -R | tail -n 1`
RANDOM_FILE_NAME=`echo "${RANDOM_FILE}" | cut -d'.' -f1`

OUTPUT_FILE=${TIMESTAMP}
OUTPUT_FILE+="_"
OUTPUT_FILE+=${RANDOM_FILE_NAME}
OUTPUT_FILE+=".jpg"

PREVIEW_FILE=${OUTPUT_FILE}

#echo ${RANDOM_FILE}
#echo ${RANDOM_FILE_NAME}
#echo ${OUTPUT_FILE}

./pi_photobooth /home/pi/Pictures/background_photos/${RANDOM_FILE} /home/pi/Pictures/preview_photos/${PREVIEW_FILE}
raspistill -ss 15000 -t 500 -o /home/pi/Pictures/foreground_photos/${OUTPUT_FILE}
#sudo timeout 2 fbi -T 2 /home/pi/Pictures/preview_photos/${PREVIEW_FILE}
#convert /home/pi/Pictures/foreground_photos/${OUTPUT_FILE} -bordercolor green -border 1x1 -matte -fill none -fuzz 35% -draw 'matte 1,1 floodfill' -shave 1x1 ${OUTPUT_MASK}
#composite -compose Dst_Over /home/pi/Pictures/background_photos/${RANDOM_FILE}  ${OUTPUT_MASK} finished.png
sync
done
