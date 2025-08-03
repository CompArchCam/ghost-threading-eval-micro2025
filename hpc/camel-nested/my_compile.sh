#! /bin/bash

mkdir -p bin

num_hash=2

gcc -g -Wall -w -O3 -fPIC camel.c ../nas-common/c_print_results.c \
    ../nas-common/c_timers.c ../nas-common/wtime.c -DPREFETCH=0 -DNUMHASH=$num_hash -o bin/camel # original (swpf best)

gcc -g -Wall -w -O3 -fPIC camel_nest2.c ../nas-common/c_print_results.c \
    ../nas-common/c_timers.c ../nas-common/wtime.c -DPREFETCH=0 -DNUMHASH=$num_hash -o bin/camel-nest # ghost best 

gcc -g -Wall -w -O3 -fPIC camel_nest4.c ../nas-common/c_print_results.c \
    ../nas-common/c_timers.c ../nas-common/wtime.c -DPREFETCH=0 -DNUMHASH=1 -o bin/camel-nest-sc # omp best 

gcc -g -Wall -w -O3 -fPIC camel.c ../nas-common/c_print_results.c \
    ../nas-common/c_timers.c ../nas-common/wtime.c -DPREFETCH=1 -DSTRIDE=1 -DNUMHASH=$num_hash -o bin/camel-man

gcc -g -Wall -w -O3 -fPIC camel_nest2.c ../nas-common/c_print_results.c \
    ../nas-common/c_timers.c ../nas-common/wtime.c -DPREFETCH=1 -DSTRIDE=1 -DNUMHASH=$num_hash -o bin/camel-nest-man

gcc -g -Wall -w -O3 -fPIC camel_nest4.c ../nas-common/c_print_results.c \
    ../nas-common/c_timers.c ../nas-common/wtime.c -DPREFETCH=1 -DSTRIDE=1 -DNUMHASH=1 -o bin/camel-nest-sc-man

gcc -g -Wall -w -O3 -std=c11 -fPIC camel_tpf.c ../thpool/thpool.c \
    ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c \
    -DPREFETCH=0 -DNUMHASH=$num_hash -pthread -o bin/camel-tpf

gcc -g -Wall -w -O3 -std=c11 -fPIC camel_nest_tpf2.c ../thpool/thpool.c \
    ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c \
    -DPREFETCH=0 -DNUMHASH=$num_hash -pthread -o bin/camel-nest-tpf

gcc -g -Wall -w -O3 -std=c11 -fPIC camel_nest_tpf4.c ../thpool/thpool.c \
    ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c \
    -DPREFETCH=0 -DNUMHASH=1 -pthread -o bin/camel-nest-sc-tpf

gcc -g -Wall -w -O3 -std=c11 -fPIC -fopenmp camel_omp.c ../nas-common/c_print_results.c \
    ../nas-common/c_timers.c ../nas-common/wtime.c -DPREFETCH=0 -DNUMHASH=$num_hash -o bin/camel-omp

gcc -g -Wall -w -O3 -std=c11 -fPIC -fopenmp camel_nest2.c ../nas-common/c_print_results.c \
    ../nas-common/c_timers.c ../nas-common/wtime.c -DOMP -DPREFETCH=0 -DNUMHASH=$num_hash -o bin/camel-nest-omp

gcc -g -Wall -w -O3 -std=c11 -fPIC -fopenmp camel_nest4.c ../nas-common/c_print_results.c \
    ../nas-common/c_timers.c ../nas-common/wtime.c -DOMP -DPREFETCH=0 -DNUMHASH=1 -o bin/camel-nest-sc-omp