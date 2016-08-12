#!/bin/bash
sudo mount /dev/sda1 /home/pi/Pictures

while true ;do
TIMESTAMP=`date +%s`
RANDOM_FILE=`ls ./background_photos | sort -R | tail -n 1`
RANDOM_FILE_NAME=`echo "${RANDOM_FILE}" | cut -d'.' -f1`

OUTPUT_FILE=${RANDOM_FILE_NAME}
OUTPUT_FILE+="_background_"
OUTPUT_FILE+=${TIMESTAMP}
OUTPUT_FILE+=".jpg"
echo ${RANDOM_FILE}
echo ${RANDOM_FILE_NAME}
echo ${OUTPUT_FILE}

./pi_photobooth background_photos/${RANDOM_FILE}
sudo raspistill -t 1000 -o /home/pi/Pictures/${OUTPUT_FILE}

done
