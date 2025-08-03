#! /bin/bash 

mkdir -p bin

gcc -g -Wall -w -O3 -fPIC -lm kangaroo.c ../nas-common/c_print_results.c \
    ../nas-common/c_timers.c ../nas-common/wtime.c -DNARRAYS=3 -DNHASH=2 -DNSWPF=0 -o bin/kangaroo

gcc -g -Wall -w -O3 -fPIC -lm kangaroo_train.c ../nas-common/c_print_results.c \
    ../nas-common/c_timers.c ../nas-common/wtime.c -DNARRAYS=3 -DNHASH=2 -DNSWPF=0 -o bin/kangaroo-train

gcc -g -Wall -w -O3 -fPIC -lm kangaroo.c ../nas-common/c_print_results.c \
    ../nas-common/c_timers.c ../nas-common/wtime.c -DNARRAYS=3 -DNHASH=2 -DNSWPF=3 -o bin/kangaroo-man

gcc -g -Wall -w -O3 -mavx2 -mfma -fPIC -lm kangaroo_tpf.c \
    ../thpool/thpool.c ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c \
    -DNARRAYS=3 -DNHASH=2 -DNSWPF=0 -pthread -lrt -o bin/kangaroo-tpf

gcc -g -Wall -w -O3 -fPIC -fopenmp -lm kangaroo_omp.c ../nas-common/c_print_results.c \
    ../nas-common/c_timers.c ../nas-common/wtime.c -DNARRAYS=3 -DNHASH=2 -DNSWPF=0 -pthread -o bin/kangaroo-omp