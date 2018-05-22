#!/bin/bash

#Simply run StopwatchViewer and EF

./../Stopwatch/build/StopwatchViewer &

./GUI/build/ElasticFusion -fo -e 250 -l ../log_files/dyson_lab.klg
