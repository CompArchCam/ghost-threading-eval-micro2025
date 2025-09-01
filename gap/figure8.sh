smt_core0=$1
smt_core1=$2

echo "Run on core $smt_core0 and $smt_core1"

./test_membw.sh cc kron $smt_core0 $smt_core1;
./test_membw.sh cc twitter $smt_core0 $smt_core1;
./test_membw.sh cc urand $smt_core0 $smt_core1;
./test_membw.sh cc road $smt_core0 $smt_core1;
./test_membw.sh cc web $smt_core0 $smt_core1;

./test_membw.sh bc kron $smt_core0 $smt_core1;
./test_membw.sh bc twitter $smt_core0 $smt_core1;
./test_membw.sh bc urand $smt_core0 $smt_core1;
./test_membw.sh bc road $smt_core0 $smt_core1;
./test_membw.sh bc web $smt_core0 $smt_core1;

./test_membw.sh bfs kron $smt_core0 $smt_core1;
./test_membw.sh bfs twitter $smt_core0 $smt_core1;
./test_membw.sh bfs urand $smt_core0 $smt_core1;
./test_membw.sh bfs road $smt_core0 $smt_core1;
./test_membw.sh bfs web $smt_core0 $smt_core1;

./test_membw.sh sssp urand $smt_core0 $smt_core1;
./test_membw.sh sssp kron $smt_core0 $smt_core1;
./test_membw.sh sssp twitter $smt_core0 $smt_core1;
./test_membw.sh sssp web $smt_core0 $smt_core1;

./test_membw.sh tc urand $smt_core0 $smt_core1;

./test_membw.sh pr kron $smt_core0 $smt_core1;
./test_membw.sh pr twitter $smt_core0 $smt_core1;
./test_membw.sh pr urand $smt_core0 $smt_core1;
./test_membw.sh pr road $smt_core0 $smt_core1;
./test_membw.sh pr web $smt_core0 $smt_core1;

./test_membw.sh tc kron $smt_core0 $smt_core1;
./test_membw.sh tc twitterU $smt_core0 $smt_core1;
./test_membw.sh tc roadU $smt_core0 $smt_core1;
./test_membw.sh tc webU $smt_core0 $smt_core1;
