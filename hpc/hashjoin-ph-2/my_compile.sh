#!/usr/bin/bash

cd src

clang -g -O3 -w npj2epb.c -c

clang -O3 -w npj2epbsw.c -DFETCHDIST=64 -DSTRIDE -c

clang -g -O3 -w npj2epb_tpf.c -c

clang -g -O3 -w -fopenmp npj2epb_omp.c -c

# Common flags and source files
COMMON_FLAGS="-g -O3 -w -pthread -lpthread -lm -std=c99"
SOURCE_FILES="main.c generator.c genzipf.c perf_counters.c cpu_mapping.c parallel_radix_join.c ../../thpool/thpool.c"
OUTPUT_DIR="../bin"

# Create output directory if it doesn't exist
mkdir -p $OUTPUT_DIR

# Link and create hj2-no executable
clang $COMMON_FLAGS npj2epb.o $SOURCE_FILES -o $OUTPUT_DIR/hj2-no

# Link and create hj2-man executable
clang $COMMON_FLAGS npj2epbsw.o $SOURCE_FILES -o $OUTPUT_DIR/hj2-man

clang $COMMON_FLAGS npj2epb_tpf.o $SOURCE_FILES -o $OUTPUT_DIR/hj2-tpf

clang $COMMON_FLAGS -fopenmp npj2epb_omp.o $SOURCE_FILES -o $OUTPUT_DIR/hj2-omp
