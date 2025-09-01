smt_core0=$1
smt_core1=$2

cd ../hpc
./figure7.sh $smt_core0 $smt_core1

cd ../gap
./figure7.sh $smt_core0 $smt_core1

cd ../figure7
./extract_energy.sh && python3 compute_saving.py && gnuplot plot.gp