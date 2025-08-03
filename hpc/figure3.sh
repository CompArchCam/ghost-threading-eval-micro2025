#!/usr/bin/bash

#----------only set these parameters----------
smt_core0=$1
smt_core1=$2
#----------only set these parameters----------

out_path=output/figure3

mkdir -p $out_path

# original camel 
echo "Running Original Camel..."
kernel_name=camel-nested/bin/camel
out_pf="$out_path/camel-baseline1.txt"
taskset -c $smt_core0 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-baseline2.txt"
taskset -c $smt_core0 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-baseline3.txt"
taskset -c $smt_core0 ./$kernel_name > $out_pf 2>&1

kernel_name=camel-nested/bin/camel-man
out_pf="$out_path/camel-swpf1.txt"
taskset -c $smt_core0 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-swpf2.txt"
taskset -c $smt_core0 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-swpf3.txt"
taskset -c $smt_core0 ./$kernel_name > $out_pf 2>&1

kernel_name=camel-nested/bin/camel-omp
out_pf="$out_path/camel-homp1.txt"
taskset -c $smt_core0,$smt_core1 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-homp2.txt"
taskset -c $smt_core0,$smt_core1 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-homp3.txt"
taskset -c $smt_core0,$smt_core1 ./$kernel_name > $out_pf 2>&1

kernel_name=camel-nested/bin/camel-tpf
out_pf="$out_path/camel-htpf1.txt"
taskset -c $smt_core0,$smt_core1 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-htpf2.txt"
taskset -c $smt_core0,$smt_core1 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-htpf3.txt"
taskset -c $smt_core0,$smt_core1 ./$kernel_name > $out_pf 2>&1

# paral camel 
echo "Running Camel Suitable for Parallelization..."
kernel_name=camel-nested/bin/camel-nest-sc
out_pf="$out_path/camel-paral-baseline1.txt"
taskset -c $smt_core0 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-paral-baseline2.txt"
taskset -c $smt_core0 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-paral-baseline3.txt"
taskset -c $smt_core0 ./$kernel_name > $out_pf 2>&1

kernel_name=camel-nested/bin/camel-nest-sc-man
out_pf="$out_path/camel-paral-swpf1.txt"
taskset -c $smt_core0 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-paral-swpf2.txt"
taskset -c $smt_core0 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-paral-swpf3.txt"
taskset -c $smt_core0 ./$kernel_name > $out_pf 2>&1

kernel_name=camel-nested/bin/camel-nest-sc-omp
out_pf="$out_path/camel-paral-homp1.txt"
taskset -c $smt_core0,$smt_core1 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-paral-homp2.txt"
taskset -c $smt_core0,$smt_core1 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-paral-homp3.txt"
taskset -c $smt_core0,$smt_core1 ./$kernel_name > $out_pf 2>&1

kernel_name=camel-nested/bin/camel-nest-sc-tpf
out_pf="$out_path/camel-paral-htpf1.txt"
taskset -c $smt_core0,$smt_core1 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-paral-htpf2.txt"
taskset -c $smt_core0,$smt_core1 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-paral-htpf3.txt"
taskset -c $smt_core0,$smt_core1 ./$kernel_name > $out_pf 2>&1

# ghost camel 
echo "Running Camel Suitable for Ghost Threading..."
kernel_name=camel-nested/bin/camel-nest
out_pf="$out_path/camel-ghost-baseline1.txt"
taskset -c $smt_core0 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-ghost-baseline2.txt"
taskset -c $smt_core0 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-ghost-baseline3.txt"
taskset -c $smt_core0 ./$kernel_name > $out_pf 2>&1

kernel_name=camel-nested/bin/camel-nest-man
out_pf="$out_path/camel-ghost-swpf1.txt"
taskset -c $smt_core0 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-ghost-swpf2.txt"
taskset -c $smt_core0 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-ghost-swpf3.txt"
taskset -c $smt_core0 ./$kernel_name > $out_pf 2>&1

kernel_name=camel-nested/bin/camel-nest-omp
out_pf="$out_path/camel-ghost-homp1.txt"
taskset -c $smt_core0,$smt_core1 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-ghost-homp2.txt"
taskset -c $smt_core0,$smt_core1 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-ghost-homp3.txt"
taskset -c $smt_core0,$smt_core1 ./$kernel_name > $out_pf 2>&1

kernel_name=camel-nested/bin/camel-nest-tpf
out_pf="$out_path/camel-ghost-htpf1.txt"
taskset -c $smt_core0,$smt_core1 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-ghost-htpf2.txt"
taskset -c $smt_core0,$smt_core1 ./$kernel_name > $out_pf 2>&1

out_pf="$out_path/camel-ghost-htpf3.txt"
taskset -c $smt_core0,$smt_core1 ./$kernel_name > $out_pf 2>&1