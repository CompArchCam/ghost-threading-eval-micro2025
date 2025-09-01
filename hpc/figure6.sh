#!/usr/bin/bash

smt_core0=$1
smt_core1=$2

./test.sh camel $smt_core0 $smt_core1
./test.sh kangaroo $smt_core0 $smt_core1
./test.sh is $smt_core0 $smt_core1
./test.sh hj2 $smt_core0 $smt_core1
./test.sh hj8 $smt_core0 $smt_core1
