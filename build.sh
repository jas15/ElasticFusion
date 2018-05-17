#!/bin/bash

mkdir deps &> /dev/null
cd deps

#This isn't really necessary as when you start with TX series just install Jetpack. 
#It'll do all the good stuff for you

#Add necessary extra repos
#version=$(lsb_release -a 2>&1)
#if [[ $version == *"14.04"* ]] ; then
#    wget http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1404/x86_64/cuda-repo-ubuntu1404_7.5-18_amd64.deb
#    sudo dpkg -i cuda-repo-ubuntu1404_7.5-18_amd64.deb
#    rm cuda-repo-ubuntu1404_7.5-18_amd64.deb
#    sudo apt-get update
#    sudo apt-get install cuda-7-5
#elif [[ $version == *"15.04"* ]] ; then
#    wget http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1504/x86_64/cuda-repo-ubuntu1504_7.5-18_amd64.deb
#    sudo dpkg -i cuda-repo-ubuntu1504_7.5-18_amd64.deb
#    rm cuda-repo-ubuntu1504_7.5-18_amd64.deb
#    sudo apt-get update
#    sudo apt-get install cuda-7-5
#elif [[ $version == *"16.04"* ]] ; then
#    wget http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1604/x86_64/cuda-repo-ubuntu1604_8.0.44-1_amd64.deb
#    sudo dpkg -i cuda-repo-ubuntu1604_8.0.44-1_amd64.deb
#    rm cuda-repo-ubuntu1604_8.0.44-1_amd64.deb
#    sudo add-apt-repository ppa:openjdk-r/ppa 
#    sudo apt-get update
#    sudo apt-get install cuda-8-0
#else
#    echo "Don't use this on anything except 14.04, 15.04, or 16.04"
#    exit
#fi

sudo add-apt-repository ppa:openjdk-r/ppa #new
sudo apt-get update #new
sudo apt-get install -y cmake-qt-gui git build-essential libusb-1.0-0-dev libudev-dev openjdk-7-jdk freeglut3-dev libglew-dev libsuitesparse-dev libeigen3-dev zlib1g-dev libjpeg-dev

#Installing Pangolin
git clone https://github.com/stevenlovegrove/Pangolin.git
cd Pangolin
mkdir build
cd build
cmake ../ -DAVFORMAT_INCLUDE_DIR="" -DCPP11_NO_BOOST=ON
make -j8
cd ../..

#Up to date OpenNI2 - can't use as need special version for Jetson TX
#git clone https://github.com/occipital/OpenNI2.git
#cd OpenNI2
#make -j8
#cd ..

#OpenNI2 that will work for TX2 (and includes libfreenect)
git clone https://github.com/mikeh9/OpenNI2-TX1.git
#Move the real folder out of the pointless folder
mv OpenNI2-TX1/OpenNI2 ./
rm -rf OpenNI2-TX1
cd OpenNI2
# Build OpenNI2
make -j4
cd ../

#Install libfreenect for Kinect use
git clone https://github.com/OpenKinect/libfreenect.git
cd libfreenect
mkdir build
cd build
cmake ..
make
sudo make install
#Build the driver for OpenNI2
cmake .. -DBUILD_OPENNI2_DRIVER=ON
make
#Copy libfreenect to OpenNI2 driver directory
cp -L lib/OpenNI2-FreenectDriver/libFreenectDriver* ../../OpenNI2/Bin/Arm-Release/OpenNI2/Drivers
#Set permissions to use sensor
sudo usermod -a -G video ubuntu

#Copy OpenNI2 libraries/include files to /usr
cd ../../OpenNI2
sudo cp -r Include /usr/include/openni2
sudo cp -r Bin/Arm-Release/OpenNI2 /usr/lib/
sudo cp Bin/Arm-Release/libOpenNI2.* /usr/lib/

#Create package config file (find location of drivers/libs/include files)
#first need to create the string
l1="prefix=/usr"
l2="exec_prefix=\${prefix}"
l3="libdir=\${exec_prefix}/lib"
l4="includedir=\${prefix}/include/openni2"
l5="Name: OpenNI2"
l6="Description: A general purpose driver for all OpenNI cameras."
l7="Version: 2.2.0.0"
l8="Cflags: -I\${includedir}"
l9="Libs: -L\${libdir} -lOpenNI2 -L\${libdir}/OpenNI2/Drivers -lDummyDevice -lOniFile -lPS1080.so"
configwrite="$l1\n$l2\n$l3\n$l4\n\n$l5\n$l6\n$l7\n$l8\n$l9"
#Then print to the correct file
sudo echo -e "$configwrite" >> /usr/lib/pkgconfig/libopenni2.pc

echo "Checking config file..."
echo "Below output should be 2.2.0.0. Something wrong if not."
pkg-config --modversion libopenni2

#Ensure rules are present in udev to handle Kinect over USB
cd Packaging/Linux
sudo cp primesense-usb.rules /etc/udev/rules.d/557-primesense-usb.rules

#Moving on...

#Actually build ElasticFusion
cd ../Core
mkdir build
cd build
cmake ../src
make -j8
# We don't really need to worry about building the GPUTest
#cd ../../GPUTest
#mkdir build
#cd build
#cmake ../src
#make -j8
cd ../../GUI
mkdir build
cd build
cmake ../src
make -j8
