#!/usr/bin/bash

smt_core0=$1
smt_core1=$2

./test_energy.sh camel $smt_core0 $smt_core1
./test_energy.sh kangaroo $smt_core0 $smt_core1
./test_energy.sh is $smt_core0 $smt_core1
./test_energy.sh hj2 $smt_core0 $smt_core1
./test_energy.sh hj8 $smt_core0 $smt_core1
