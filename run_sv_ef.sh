#!/bin/bash

./cg_build.sh

./../Stopwatch/build/StopwatchViewer &

./GUI/build/ElasticFusion -l ../dyson_lab.klg
