#!/bin/bash

#Simply run StopwatchViewer and EF

./../Stopwatch/build/StopwatchViewer &

#Run ElasticFusion with fast odometry, quitting at the end of a log file, ending the log at 250 frames
./GUI/build/ElasticFusion -fo -q -e 50 -l ../log_files/dyson_lab.klg
