#!/bin/bash

## Arguments
# $1 = string of flags
# $2 = flag description

## Run 1 - living room 2
runname="_logfile1"
runname=$2$runname
fx=481.2
fy=480
cx=319.5
cy=239.5
logfile=../Datasets/livingRoom2/new_klg/livingRoom2.klg

./other_scripts/test_run.sh $runname $fx $fy $cx $cy "$1" $logfile

## Run 2 - office 1
runname="_logfile2"
runname=$2$runname
fx=481.2
fy=480
cx=319.5
cy=239.5
logfile=../Datasets/officeRoom1/new_klg/officeRoom1.klg

./other_scripts/test_run.sh $runname $fx $fy $cx $cy "$1" $logfile

## Run 3 - living room 3
runname="_logfile3"
runname=$2$runname
fx=481.2
fy=480
cx=319.5
cy=239.5
logfile=../Datasets/livingRoom3/new_klg/livingRoom3.klg

./other_scripts/test_run.sh $runname $fx $fy $cx $cy "$1" $logfile
