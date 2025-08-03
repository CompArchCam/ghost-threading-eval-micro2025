DEFAULT: all
BECNHES :=
OptCMD ?= disassemble

BENCHES += gapbs.bc.kron.train
gapbs.bc.kron.train:
	optiwise $(OptCMD) --name=$@ 
BENCHES += gapbs.bc.twitter.train
gapbs.bc.twitter.train:
	optiwise $(OptCMD) --name=$@ 
BENCHES += gapbs.bc.urand.train
gapbs.bc.urand.train:
	optiwise $(OptCMD) --name=$@ 
BENCHES += gapbs.bc.web.train
gapbs.bc.web.train:
	optiwise $(OptCMD) --name=$@ 
BENCHES += gapbs.bc.road.train
gapbs.bc.road.train:
	optiwise $(OptCMD) --name=$@ 

BENCHES += gapbs.bfs.kron.train
gapbs.bfs.kron.train:
	optiwise $(OptCMD) --name=$@ 
BENCHES += gapbs.bfs.twitter.train
gapbs.bfs.twitter.train:
	optiwise $(OptCMD) --name=$@ 
BENCHES += gapbs.bfs.urand.train
gapbs.bfs.urand.train:
	optiwise $(OptCMD) --name=$@ 
BENCHES += gapbs.bfs.web.train
gapbs.bfs.web.train:
	optiwise $(OptCMD) --name=$@ 
BENCHES += gapbs.bfs.road.train
gapbs.bfs.road.train:
	optiwise $(OptCMD) --name=$@ 

BENCHES += gapbs.tc.kron.train
gapbs.tc.kron.train:
	optiwise $(OptCMD) --name=$@ 
BENCHES += gapbs.tc.twitter.train
gapbs.tc.twitter.train:
	optiwise $(OptCMD) --name=$@ 
BENCHES += gapbs.tc.urand.train
gapbs.tc.urand.train:
	optiwise $(OptCMD) --name=$@ 
BENCHES += gapbs.tc.web.train
gapbs.tc.web.train:
	optiwise $(OptCMD) --name=$@ 
BENCHES += gapbs.tc.road.train
gapbs.tc.road.train:
	optiwise $(OptCMD) --name=$@ 

BENCHES += gapbs.cc.kron.train
gapbs.cc.kron.train:
	optiwise $(OptCMD) --name=$@
BENCHES += gapbs.cc.twitter.train
gapbs.cc.twitter.train:
	optiwise $(OptCMD) --name=$@
BENCHES += gapbs.cc.urand.train
gapbs.cc.urand.train:
	optiwise $(OptCMD) --name=$@
BENCHES += gapbs.cc.web.train
gapbs.cc.web.train:
	optiwise $(OptCMD) --name=$@
BENCHES += gapbs.cc.road.train
gapbs.cc.road.train:
	optiwise $(OptCMD) --name=$@

BENCHES += gapbs.pr.kron.train
gapbs.pr.kron.train:
	optiwise $(OptCMD) --name=$@
BENCHES += gapbs.pr.twitter.train
gapbs.pr.twitter.train:
	optiwise $(OptCMD) --name=$@
BENCHES += gapbs.pr.urand.train
gapbs.pr.urand.train:
	optiwise $(OptCMD) --name=$@
BENCHES += gapbs.pr.web.train
gapbs.pr.web.train:
	optiwise $(OptCMD) --name=$@
BENCHES += gapbs.pr.road.train
gapbs.pr.road.train:
	optiwise $(OptCMD) --name=$@

BENCHES += gapbs.sssp.kron.train
gapbs.sssp.kron.train:
	optiwise $(OptCMD) --name=$@
BENCHES += gapbs.sssp.twitter.train
gapbs.sssp.twitter.train:
	optiwise $(OptCMD) --name=$@
BENCHES += gapbs.sssp.urand.train
gapbs.sssp.urand.train:
	optiwise $(OptCMD) --name=$@
BENCHES += gapbs.sssp.web.train
gapbs.sssp.web.train:
	optiwise $(OptCMD) --name=$@
BENCHES += gapbs.sssp.road.train
gapbs.sssp.road.train:
	optiwise $(OptCMD) --name=$@

BENCHES += hpc.camel.train
hpc.camel.train:
	optiwise $(OptCMD) --name=$@

BENCHES += hpc.kangaroo.train
hpc.kangaroo.train:
	optiwise $(OptCMD) --name=$@

BENCHES += hpc.is.train
hpc.is.train:
	optiwise $(OptCMD) --name=$@

BENCHES += hpc.hj2.train
hpc.hj2.train:
	optiwise $(OptCMD) --name=$@

BENCHES += hpc.hj8.train
hpc.hj8.train:
	optiwise $(OptCMD) --name=$@


TARGETS ?= $(filter %train,$(BENCHES)) 

all: $(TARGETS)