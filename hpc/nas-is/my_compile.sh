mkdir -p bin

gcc -O3 -g -w -std=c11 is_tpf.c ../thpool/thpool.c ../nas-common/c_print_results.c \
    ../nas-common/c_timers.c ../nas-common/wtime.c ../nas-common/c_randdp.c -lm -pthread -o bin/is-tpf

gcc -O3 -g -w -std=c11 -fopenmp is_omp.c ../nas-common/c_print_results.c \
    ../nas-common/c_timers.c ../nas-common/wtime.c ../nas-common/c_randdp.c -lm -o bin/is-omp

gcc -O3 -g -w -std=c11 is.c ../nas-common/c_print_results.c \
    ../nas-common/c_timers.c ../nas-common/wtime.c -o bin/is-no

gcc -O3 -g -w -std=c11 is.c ../nas-common/c_print_results.c \
    ../nas-common/c_timers.c ../nas-common/wtime.c -DTRAIN -o bin/is-no-train

gcc -O3 -g -w -std=c11 isswpf.c -DFETCHDIST=64 -DSTRIDE ../nas-common/c_print_results.c \
    ../nas-common/c_timers.c ../nas-common/wtime.c -o bin/is-man