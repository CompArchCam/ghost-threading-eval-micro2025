smt_core0=$1
smt_core1=$2

echo "Run on core $smt_core0 and $smt_core1"

./test.sh cc kron $smt_core0 $smt_core1;
./test.sh cc twitter $smt_core0 $smt_core1;
./test.sh cc urand $smt_core0 $smt_core1;
./test.sh cc road $smt_core0 $smt_core1;
./test.sh cc web $smt_core0 $smt_core1;

./test.sh bc kron $smt_core0 $smt_core1;
./test.sh bc twitter $smt_core0 $smt_core1;
./test.sh bc urand $smt_core0 $smt_core1;
./test.sh bc road $smt_core0 $smt_core1;
./test.sh bc web $smt_core0 $smt_core1;

./test.sh bfs kron $smt_core0 $smt_core1;
./test.sh bfs twitter $smt_core0 $smt_core1;
./test.sh bfs urand $smt_core0 $smt_core1;
./test.sh bfs road $smt_core0 $smt_core1;
./test.sh bfs web $smt_core0 $smt_core1;

./test.sh sssp urand $smt_core0 $smt_core1;
./test.sh sssp kron $smt_core0 $smt_core1;
./test.sh sssp twitter $smt_core0 $smt_core1;
./test.sh sssp web $smt_core0 $smt_core1;

./test.sh tc urand $smt_core0 $smt_core1;

./test.sh pr kron $smt_core0 $smt_core1;
./test.sh pr twitter $smt_core0 $smt_core1;
./test.sh pr urand $smt_core0 $smt_core1;
./test.sh pr road $smt_core0 $smt_core1;
./test.sh pr web $smt_core0 $smt_core1;

./test.sh tc kron $smt_core0 $smt_core1;
./test.sh tc twitterU $smt_core0 $smt_core1;
./test.sh tc roadU $smt_core0 $smt_core1;
./test.sh tc webU $smt_core0 $smt_core1;
