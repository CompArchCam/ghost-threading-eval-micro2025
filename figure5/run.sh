smt_core0=$1
smt_core1=$2

cd ../gap
./figure5.sh $smt_core0 $smt_core1

cd ../hpc
./figure5.sh $smt_core0 $smt_core1

cd ../figure5
./extract_time.sh && python3 compute_speedup.py