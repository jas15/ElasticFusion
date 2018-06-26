#!/bin/bash

#Simply run StopwatchViewer and EF

./../Stopwatch/build/StopwatchViewer &

#./GUI/build/ElasticFusion -q -icl -l ../../Datasets/livingRoom3/new_klg/livingRoom3.klg
./GUI/build/ElasticFusion -q -l ../../Datasets/dyson_lab.klg

### -nso NOTE disable SO(3) pre-alignment in tracking
### -fo  NOTE fast odometry (single level pyramid)
### -o   NOTE open loop mode

## Other things that we could look to change
#-c  <><> Surfel confidence threshold (default *10*).
#-d  <><> Cutoff distance for depth processing (default *3*m).
#-ie <><> Local loop closure residual threshold (default *5e-05*).
#-ic <><> Local loop closure inlier threshold (default *35000*).
#-cv <><> Local loop closure covariance threshold (default *1e-05*).
#-pt <><> Global loop closure photometric threshold (default *115*).
#-ft <><> Fern encoding threshold (default *0.3095*).

## More flags that could be important...
#-icl : Enable this if using the [ICL-NUIM](http://www.doc.ic.ac.uk/~ahanda/VaFRIC/iclnuim.html) dataset (flips normals to account for negative focal length on that data).
#-rl  : Enable relocalisation.
#-ftf : Do frame-to-frame RGB tracking. 
#-sc  : Showcase mode (minimal GUI).
