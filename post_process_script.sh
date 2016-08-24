#!/bin/bash
#sudo mount /dev/sda1 /home/pi/Pictures -o dmask=000,fmask=111
BACKGROUND_FILES=${1}/background_photos/*
for fb in ${BACKGROUND_FILES}
do
	FILE_NAME=${fb%.*}
	FILE_NAME=${FILE_NAME##*/}
	echo ${FILE_NAME}
	FOREGROUND_FILES=`find ${1}/foreground_photos -name "*_${FILE_NAME}.jpg"`
	for fg in ${FOREGROUND_FILES}
	do
		FINISHED_NAME=${2}/${FILE_NAME}_finished.png
		FINISHED_MASK=${2}/${FILE_NAME}_mask.png
		convert ${fg} -bordercolor green -border 1x1 -matte -fill none -fuzz 15% -draw 'matte 1,1 floodfill' -shave 1x1 ${FINISHED_MASK}
		composite -compose Dst_Over ${fb} ${FINISHED_MASK} ${FINISHED_NAME}
	done
done
