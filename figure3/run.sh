smt_core0=$1
smt_core1=$2

cd ../hpc
./figure3.sh $smt_core0 $smt_core1

cd ../figure3
./extract_time.sh && python3 compute_speedup.py
