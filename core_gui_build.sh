#!/bin/bash

#Make before running

cd Core/build/
make clean
make -j4

cd ../../

cd GUI/build/
make clean
make -j4

cd ../../
