smt_core0=$1
smt_core1=$2

echo "Run on core $smt_core0 and $smt_core1"

./test_energy.sh cc kron $smt_core0 $smt_core1;
./test_energy.sh cc twitter $smt_core0 $smt_core1;
./test_energy.sh cc urand $smt_core0 $smt_core1;
./test_energy.sh cc road $smt_core0 $smt_core1;
./test_energy.sh cc web $smt_core0 $smt_core1;

./test_energy.sh bc kron $smt_core0 $smt_core1;
./test_energy.sh bc twitter $smt_core0 $smt_core1;
./test_energy.sh bc urand $smt_core0 $smt_core1;
./test_energy.sh bc road $smt_core0 $smt_core1;
./test_energy.sh bc web $smt_core0 $smt_core1;

./test_energy.sh bfs kron $smt_core0 $smt_core1;
./test_energy.sh bfs twitter $smt_core0 $smt_core1;
./test_energy.sh bfs urand $smt_core0 $smt_core1;
./test_energy.sh bfs road $smt_core0 $smt_core1;
./test_energy.sh bfs web $smt_core0 $smt_core1;

./test_energy.sh sssp urand $smt_core0 $smt_core1;
./test_energy.sh sssp kron $smt_core0 $smt_core1;
./test_energy.sh sssp twitter $smt_core0 $smt_core1;
./test_energy.sh sssp web $smt_core0 $smt_core1;

./test_energy.sh tc urand $smt_core0 $smt_core1;

./test_energy.sh pr kron $smt_core0 $smt_core1;
./test_energy.sh pr twitter $smt_core0 $smt_core1;
./test_energy.sh pr urand $smt_core0 $smt_core1;
./test_energy.sh pr road $smt_core0 $smt_core1;
./test_energy.sh pr web $smt_core0 $smt_core1;

./test_energy.sh tc kron $smt_core0 $smt_core1;
./test_energy.sh tc twitterU $smt_core0 $smt_core1;
./test_energy.sh tc roadU $smt_core0 $smt_core1;
./test_energy.sh tc webU $smt_core0 $smt_core1;
