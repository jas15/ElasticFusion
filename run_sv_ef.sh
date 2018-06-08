#!/bin/bash

#Simply run StopwatchViewer and EF

#./../Stopwatch/build/StopwatchViewer &

#Run ElasticFusion with fast odometry, quitting at the end of a log file, ending the log at 250 frames
#./GUI/build/ElasticFusion -fo -q -e 250 -l ../log_files/dyson_lab.klg
#./GUI/build/ElasticFusion -fo -q -e 250 -icl -l ../Datasets/livingRoom2/new_klg/livingRoom2.klg

###./GUI/build/ElasticFusion -cal FX FY CX CY -q -l ../log_files/dyson_lab.klg NOTE for calibration !
### -nso NOTE disable SO(3) pre-alignment in tracking
### -fo  NOTE fast odometry (single level pyramid)
### -o   NOTE open loop mode

## All of the possible combinations of flags to use. Give an accuracy / speed tradeoff !
# Should be able to call like ./test_all "-list -of -flags" briefDesc :

./test_all.sh "           " none
./test_all.sh "       -nso" noSO3
./test_all.sh "   -fo     " fastOdom
./test_all.sh "   -fo -nso" fastOdom_noSO3
./test_all.sh "-o         " openLoop
./test_all.sh "-o     -nso" openLoop_noSO3
./test_all.sh "-o -fo     " openLoop_fastOdom
./test_all.sh "-o -fo -nso" openLoop_fastOdom_noSO3

#"or disable the tracking pyramid" IN MAINCONTROLLER.CPP towards the bottom of the big commented out bit (pass in false)

## Other things that we could look to change
#-c  <><> Surfel confidence threshold (default *10*).
#-d  <><> Cutoff distance for depth processing (default *3*m).
#-ie <><> Local loop closure residual threshold (default *5e-05*).
#-ic <><> Local loop closure inlier threshold (default *35000*).
#-cv <><> Local loop closure covariance threshold (default *1e-05*).
#-pt <><> Global loop closure photometric threshold (default *115*).
#-ft <><> Fern encoding threshold (default *0.3095*).

## More flags that could be important... What about -sc?
#-icl : Enable this if using the [ICL-NUIM](http://www.doc.ic.ac.uk/~ahanda/VaFRIC/iclnuim.html) dataset (flips normals to account for negative focal length on that data).
#-rl  : Enable relocalisation.
#-ftf : Do frame-to-frame RGB tracking. 
#-sc  : Showcase mode (minimal GUI).
