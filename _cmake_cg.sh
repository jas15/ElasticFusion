#!/bin/bash

cd Core/build/
rm -rf *
cmake ../src
make -j4

cd ../../

cd GUI/build/
rm -rf *
cmake ../src
make -j4
