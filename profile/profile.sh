#!/usr/bin/bash

mkdir -p log 

cd ../gap
make clean
make SERIAL=1 -j
cd ../profile

make -f exec_all.mk OptCMD=sample

make -f exec_all.mk -j OptCMD=count 

make -f static_all.mk

make -f static_all.mk OptCMD=analyze

make -f static_all.mk OptCMD=gui
