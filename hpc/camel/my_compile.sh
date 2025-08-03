#! /bin/bash

mkdir -p bin

gcc -g -Wall -w -O3 -fPIC camel.c ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c -DPREFETCH=0 -DNUMHASH=2 -o bin/camel

gcc -g -Wall -w -O3 -fPIC camel.c ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c -DMAX_KEY=8388608 -DPREFETCH=0 -DNUMHASH=2 -o bin/camel-train

gcc -g -Wall -w -O3 -fPIC camel.c ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c -DPREFETCH=1 -DSTRIDE=1 -DNUMHASH=2 -o bin/camel-man

gcc -g -Wall -w -O3 -std=c11 -fPIC camel_tpf.c ../thpool/thpool.c ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c -DPREFETCH=0 -DNUMHASH=2 -pthread -o bin/camel-tpf

gcc -g -Wall -w -O3 -std=c11 -fPIC camel_tpf.c ../thpool/thpool.c ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c -DMEMBW -DPREFETCH=0 -DNUMHASH=2 -pthread -o bin/camel-tpf-membw

gcc -g -Wall -w -O3 -std=c11 -fPIC -fopenmp camel_omp.c ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c -DPREFETCH=0 -DNUMHASH=2 -o bin/camel-omp