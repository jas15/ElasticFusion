#!/bin/bash

#Delete the CMake files and then rerun cmake, build

cd Core/build/
rm -rf *
cmake ../src
make -j4

cd ../../

cd GUI/build/
rm -rf *
cmake ../src
make -j4
