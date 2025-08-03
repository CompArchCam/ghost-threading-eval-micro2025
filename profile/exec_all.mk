DEFAULT: all
BECNHES :=
OptCMD ?= run --gui

BENCHES += hpc.camel.train
hpc.camel.train:
	optiwise $(OptCMD) --name=$@ -- ../hpc/camel/bin/camel-train > log/$@.log 2 > log/$@.err

BENCHES += hpc.kangaroo.train
hpc.kangaroo.train:
	optiwise $(OptCMD) --name=$@ -- ../hpc/kangaroo/bin/kangaroo-train > log/$@.log 2 > log/$@.err

BENCHES += hpc.is.train
hpc.is.train:
	optiwise $(OptCMD) --name=$@ -- ../hpc/nas-is/bin/is-no-train > log/$@.log 2 > log/$@.err

BENCHES += hpc.hj2.train
hpc.hj2.train:
	optiwise $(OptCMD) --name=$@ -- ../hpc/hashjoin-ph-2/bin/hj2-no -a NPO_st -r 3200000 -s 3200000 > log/$@.log 2 > log/$@.err

BENCHES += hpc.hj8.train
hpc.hj8.train:
	optiwise $(OptCMD) --name=$@ -- ../hpc/hashjoin-ph-8/bin/hj8-no -a NPO_st -r 3200000 -s 3200000 > log/$@.log 2 > log/$@.err

BENCHES += gapbs.bc.kron.train
gapbs.bc.kron.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/bc -n 1 -f ../gap/benchmark/graphs/kron-train.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.bc.twitter.train
gapbs.bc.twitter.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/bc -n 1 -f ../gap/benchmark/graphs/twitter-train.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.bc.urand.train
gapbs.bc.urand.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/bc -n 1 -f ../gap/benchmark/graphs/urand-train.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.bc.web.train
gapbs.bc.web.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/bc -n 1 -f ../gap/benchmark/graphs/web-train.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.bc.road.train
gapbs.bc.road.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/bc -n 1 -f ../gap/benchmark/graphs/road-train.sg > log/$@.log 2 > log/$@.err

BENCHES += gapbs.bfs.kron.train
gapbs.bfs.kron.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/bfs -n 1 -f ../gap/benchmark/graphs/kron-train.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.bfs.twitter.train
gapbs.bfs.twitter.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/bfs -n 1 -f ../gap/benchmark/graphs/twitter-train.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.bfs.urand.train
gapbs.bfs.urand.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/bfs -n 1 -f ../gap/benchmark/graphs/urand-train.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.bfs.web.train
gapbs.bfs.web.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/bfs -n 1 -f ../gap/benchmark/graphs/web-train.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.bfs.road.train
gapbs.bfs.road.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/bfs -n 1 -f ../gap/benchmark/graphs/road-train.sg > log/$@.log 2 > log/$@.err


BENCHES += gapbs.cc.kron.train
gapbs.cc.kron.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/cc -n 1 -f ../gap/benchmark/graphs/kron-train.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.cc.twitter.train
gapbs.cc.twitter.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/cc -n 1 -f ../gap/benchmark/graphs/twitter-train.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.cc.urand.train
gapbs.cc.urand.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/cc -n 1 -f ../gap/benchmark/graphs/urand-train.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.cc.web.train
gapbs.cc.web.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/cc -n 1 -f ../gap/benchmark/graphs/web-train.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.cc.road.train
gapbs.cc.road.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/cc -n 1 -f ../gap/benchmark/graphs/road-train.sg > log/$@.log 2 > log/$@.err

BENCHES += gapbs.pr.kron.train
gapbs.pr.kron.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/pr -n 1 -f ../gap/benchmark/graphs/kron-train.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.pr.twitter.train
gapbs.pr.twitter.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/pr -n 1 -f ../gap/benchmark/graphs/twitter-train.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.pr.urand.train
gapbs.pr.urand.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/pr -n 1 -f ../gap/benchmark/graphs/urand-train.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.pr.web.train
gapbs.pr.web.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/pr -n 1 -f ../gap/benchmark/graphs/web-train.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.pr.road.train
gapbs.pr.road.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/pr -n 1 -f ../gap/benchmark/graphs/road-train.sg > log/$@.log 2 > log/$@.err

BENCHES += gapbs.sssp.kron.train
gapbs.sssp.kron.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/sssp -n 1 -f ../gap/benchmark/graphs/kron-train.wsg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.sssp.twitter.train
gapbs.sssp.twitter.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/sssp -n 1 -f ../gap/benchmark/graphs/twitter-train.wsg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.sssp.urand.train
gapbs.sssp.urand.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/sssp -n 1 -f ../gap/benchmark/graphs/urand-train.wsg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.sssp.web.train
gapbs.sssp.web.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/sssp -n 1 -f ../gap/benchmark/graphs/web-train.wsg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.sssp.road.train
gapbs.sssp.road.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/sssp -n 1 -f ../gap/benchmark/graphs/road-train.wsg > log/$@.log 2 > log/$@.err

BENCHES += gapbs.tc.kron.train
gapbs.tc.kron.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/tc -n 1 -f ../gap/benchmark/graphs/kron-trainU.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.tc.twitter.train
gapbs.tc.twitter.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/tc -n 1 -f ../gap/benchmark/graphs/twitter-trainU.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.tc.urand.train
gapbs.tc.urand.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/tc -n 1 -f ../gap/benchmark/graphs/urand-trainU.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.tc.web.train
gapbs.tc.web.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/tc -n 1 -f ../gap/benchmark/graphs/web-trainU.sg > log/$@.log 2 > log/$@.err
BENCHES += gapbs.tc.road.train
gapbs.tc.road.train:
	optiwise $(OptCMD) --name=$@ -- ../gap/tc -n 1 -f ../gap/benchmark/graphs/road-trainU.sg > log/$@.log 2 > log/$@.err

TARGETS ?= $(filter %.train,$(BENCHES)) 

all: $(TARGETS)