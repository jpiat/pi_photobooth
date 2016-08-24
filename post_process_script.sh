#!/bin/bash
sudo mount /dev/sda1 /home/pi/Pictures -o dmask=000,fmask=111
BACKGROUND_FILES=/home/pi/Pictures/background_photos/*
for fb in ${BACKGROUND_FILES}
do
	FILE_NAME=${fb%.*}
	FILE_NAME=${FILE_NAME##*/}
	echo ${FILE_NAME}
	FOREGROUND_FILES= /home/pi/Pictures/foreground_photos/*_${FILE_NAME}.jpg
	for fg in ${FOREGROUND_FILE}
	do
		FINISHED_NAME=${1}/${FILE_NAME}_finished.png
		FINISHED_MASK=$ {1}/${FILE_NAME}_mask.png
		convert ${fg} -bordercolor green -border 1x1 -matte -fill none -fuzz 15% -draw 'matte 1,1 floodfill' -shave 1x1 ${FINISHED_MASK}
		composite -compose Dst_Over ${fb} ${OUTPUT_MASK} finished.png
	done
done
