#!/bin/bash
sudo mount /dev/sda1 /home/pi/Pictures -o dmask=000,fmask=111

fbi -noverbose -a -u -t 4 /home/pi/Pictures/preview_photos/*
