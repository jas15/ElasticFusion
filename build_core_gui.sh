#!/bin/bash

# Build CORE
cd Core/
mkdir build
cd build/
cmake ../src #Here is the SuiteSparse issue. Working on it
make
cd ../../

#Build GUI
#cd GUI/
#mkdir build
#cd build/
#cmake ../src
#make
