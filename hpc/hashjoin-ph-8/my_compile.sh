#!/usr/bin/bash

cd src

OUTPUT_DIR="../bin"
mkdir -p $OUTPUT_DIR

# compile no
clang -O3 -w -g npj8epb.c -c 
clang -O3 -w npj8epb.o main.c generator.c genzipf.c perf_counters.c cpu_mapping.c parallel_radix_join.c \
    -lpthread -lm -std=c99  -o $OUTPUT_DIR/hj8-no

#compile man
clang -O3 -w npj8epbsw.c -DNUMPREFETCHES=3 -DSTRIDE -c 
clang -O3 -w npj8epbsw.o main.c generator.c genzipf.c perf_counters.c cpu_mapping.c parallel_radix_join.c \
    -lpthread -lm -std=c99  -o $OUTPUT_DIR/hj8-man

# compile htpf
clang -g -O3 -w npj8epb_tpf.c -c 
clang -g -O3 -w npj8epb_tpf.o main.c generator.c genzipf.c perf_counters.c cpu_mapping.c \
    parallel_radix_join.c ../../thpool/thpool.c -pthread -lpthread -lm -std=c99  -o $OUTPUT_DIR/hj8-tpf

#compile omp
clang -g -O3 -w -fopenmp npj8epb_omp.c -c 
clang -g -O3 -w -fopenmp npj8epb_omp.o main.c generator.c genzipf.c perf_counters.c cpu_mapping.c \
    parallel_radix_join.c -pthread -lpthread -lm -std=c99  -o $OUTPUT_DIR/hj8-omp
