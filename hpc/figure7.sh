#!/usr/bin/bash

smt_core0=$1
smt_core1=$2

./test_membw.sh camel $smt_core0 $smt_core1
./test_membw.sh kangaroo $smt_core0 $smt_core1
./test_membw.sh is $smt_core0 $smt_core1
./test_membw.sh hj2 $smt_core0 $smt_core1
./test_membw.sh hj8 $smt_core0 $smt_core1
