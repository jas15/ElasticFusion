#!/bin/bash

cd /home/nvidia/Elasticfusion/

#Need to install QT 4.x+ first
sudo apt-get install -y qt4-dev-tools
#Also need NCurses library
sudo apt-get install -y libncurses-dev

#Then we can clone n install etccc
git clone https://github.com/mp3guy/Stopwatch
cd Stopwatch

#Make the build directory
mkdir build
cd build/

cmake ../src/
make -j4

#After this point all should be good to run ./StopwatchViewer
