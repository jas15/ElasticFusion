#!/bin/bash

cd Core/build/
make clean
make -j4

cd ../../

cd GUI/build/
make clean
make -j4
