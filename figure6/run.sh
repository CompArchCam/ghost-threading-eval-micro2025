smt_core0=$1
smt_core1=$2

cd ../hpc
./figure6.sh $smt_core0 $smt_core1

cd ../gap
./figure6.sh $smt_core0 $smt_core1

cd ../figure5
./extract_energy.sh && python3 compute_saving.py