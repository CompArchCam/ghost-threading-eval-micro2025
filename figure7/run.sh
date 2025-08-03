#!/usr/bin/bash

# set core used to generate memory bandwidth pressure, this uses core 2, 4, 6 ... 14 
start_core=2
end_core=14
step=2
#----------do not change parameters below------------

smt_core0=$1
smt_core1=$2

# generate membw 
echo "generating 21 GB/s memory bandwidth pressure"
for ((i = $start_core; i <= $end_core; i=i+$step)); do
    membw -c $i -b 3072 --read-mod-write &
    multi_pids+=($!)
done

cd ../gap
./figure7.sh $smt_core0 $smt_core1

cd ../hpc
./figure7.sh $smt_core0 $smt_core1

# stop generating membw 
echo "Execution completed."
echo "Killing membw jobs..."
for pid in "${multi_pids[@]}"; do
    kill $pid 2>/dev/null
done
wait 

cd ../figure5
./extract_time.sh && python3 compute_speedup.py