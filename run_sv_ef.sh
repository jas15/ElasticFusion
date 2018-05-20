#!/bin/bash

#Simply run StopwatchViewer and EF

./../Stopwatch/build/StopwatchViewer &

./GUI/build/ElasticFusion -fo -l ../log_files/dyson_lab.klg
