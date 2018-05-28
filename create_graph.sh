#!/bin/bash

cd ../Datasets/ate_scripts/

python evaluate_ate.py --verbose --plot tmp.png ../livingRoom2/livingRoom2PNG/livingRoom2.gt.freiburg ../livingRoom2/new_klg/livingRoom2.klg__logfile1.freiburg
