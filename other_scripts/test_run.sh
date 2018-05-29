#!/bin/bash

## Arguments
# $1 = run name (appending in freiburg files)
# $2 = FX
# $3 = FY
# $4 = FZ
# $5 = FD (calibrations)
# $6 = other flags for ElasticFusion
# $7 = log file location
#cd ..

echo "Running Stopwatch..."
./../Stopwatch/build/StopwatchViewer &

echo "Running ElasticFusion for the first time..."
./GUI/build/ElasticFusion -q $6 -icl -l $7
#./GUI/build/ElasticFusion -q $6 -e 300 -icl -l $7
#./GUI/build/ElasticFusion -cal $2 $3 $4 $5 -q $6 -icl -l $7

echo "ElasticFusion complete for speed test. Waiting to close StopwatchViewer."

read -p "Once closed, press Enter to continue."

echo "Running ElasticFusion for the second time..."
./GUI/build/ElasticFusion -q $6 -icl -l $7
#./GUI/build/ElasticFusion -cal $2 $3 $4 $5 -q $6 -icl -l $7
echo "Complete."

echo "Moving output files..."
## Move the produced files to places with good names
# move freiburg
mv $7.freiburg $7_$1.freiburg
# move ply
mv $7.ply $7_$1.ply
# move refresh rate tally
mv ../log_files/refreshRate.txt ../log_files/refreshRate_$1.txt

echo "Complete."
