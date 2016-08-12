#!/bin/sh

BASE_DIR=`pwd`

sudo apt-get update
sudo apt-get -y install cmake gcc g++ libx11-dev libxt-dev libxext-dev libgraphicsmagick1-dev liblz4-dev
sudo apt-get -y  install wiringpi
#required by OpenCV
sudo apt-get -y install libjpeg8-dev libjasper-dev libpng12-dev
sudo apt-get -y install libavcodec-dev libavformat-dev libswscale-dev libv4l-dev
sudo apt-get -y install libatlas-base-dev gfortran
sudo apt-get -y install libgtk2.0-dev
sudo apt-get -y install libdc1394-22-dev libilmbase6 openexr libopenexr-dev
#end of required by OpenCV
sudo apt-get -y install libsdl1.2-dev


if [ ! -d dependencies ]; then
	mkdir dependencies
fi

cd dependencies

if [ ! -d opencv ]; then
	git clone https://github.com/Itseez/opencv.git
	cd opencv/
	git checkout tags/3.1.0
fi

cd ${BASE_DIR}/dependencies

if [ ! -d userland ]; then
	git clone https://github.com/raspberrypi/userland.git
else
	cd userland
	git pull
	cd ..
fi

cd ${BASE_DIR}/dependencies

if [ ! -d robidouille ]; then
	git clone https://github.com/jpiat/robidouille.git
else
	cd robidouille
	git pull
	cd ..
fi

cd ${BASE_DIR}/dependencies

cd opencv/
git checkout tags/3.1.0
mkdir release
cd release
cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local -D ENABLE_NEON=ON ..
make -j4
sudo make install

cd ${BASE_DIR}/dependencies

cd userland
USERLAND_PWD=${PWD}
./buildme
cd ..

echo ${USERLAND_PWD}
cd robidouille/raspicam_cv
mkdir objs
USERLAND_ROOT=${USERLAND_PWD} make
sudo make install

