#!/bin/bash

# Build CORE
cd Core/
mkdir build
cd build/
cmake ../src
make
cd ../../

#Build GUI
cd GUI/
mkdir build
cd build/
cmake ../src
make
