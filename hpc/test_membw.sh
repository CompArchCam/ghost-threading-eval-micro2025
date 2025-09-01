#!/usr/bin/bash

#----------only set these parameters----------
exe_baslines=1
exe_homp=1
exe_swpf=1
exe_htpf=1

out_path=$(pwd)/output/figure8
#----------only set these parameters----------

kernel_name=$1
smt_core0=$2
smt_core1=$3

kernel_baseline="$kernel_name-no"
kernel_homp="$kernel_name-omp"
kernel_swpf="$kernel_name-man"
kernel_htpf="$kernel_name-tpf"

if [ "$kernel_name" == "camel" ]; then
	cd camel/bin
	kernel_baseline=$kernel_name
	kernel_htpf="$kernel_name-tpf-membw"
elif [ "$kernel_name" == "kangaroo" ]; then
	exe_homp=0
	cd kangaroo/bin
	kernel_baseline=$kernel_name
elif [ "$kernel_name" == "is" ]; then
	exe_homp=0
	cd nas-is/bin
elif [ "$kernel_name" == "hj2" ]; then
	cd hashjoin-ph-2/bin
elif [ "$kernel_name" == "hj8" ]; then
	cd hashjoin-ph-8/bin
fi

mkdir -p $out_path

if [ "$exe_baslines" == "1" ]; then
	out_pf="$out_path/$kernel_name-baseline.txt"
	echo "Baseline: $out_pf."
    echo "---------------------------"
	if [ "$kernel_name" == "hj2" ]; then
		out_pf="$out_path/$kernel_name-baseline1.txt"
		time taskset -c $smt_core0 ./$kernel_baseline -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 

		out_pf="$out_path/$kernel_name-baseline2.txt"
		time taskset -c $smt_core0 ./$kernel_baseline -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 

		out_pf="$out_path/$kernel_name-baseline3.txt"
		time taskset -c $smt_core0 ./$kernel_baseline -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 
	elif [ "$kernel_name" == "hj8" ]; then
		out_pf="$out_path/$kernel_name-baseline1.txt"
		time taskset -c $smt_core0 ./$kernel_baseline -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 

		out_pf="$out_path/$kernel_name-baseline2.txt"
		time taskset -c $smt_core0 ./$kernel_baseline -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 

		out_pf="$out_path/$kernel_name-baseline3.txt"
		time taskset -c $smt_core0 ./$kernel_baseline -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 
	else
		out_pf="$out_path/$kernel_name-baseline1.txt"
		time taskset -c $smt_core0 ./$kernel_baseline > $out_pf 2>&1

		out_pf="$out_path/$kernel_name-baseline2.txt"
		time taskset -c $smt_core0 ./$kernel_baseline > $out_pf 2>&1

		out_pf="$out_path/$kernel_name-baseline3.txt"
		time taskset -c $smt_core0 ./$kernel_baseline > $out_pf 2>&1
	fi
fi 

if [ "$exe_homp" == "1" ]; then
	out_pf="$out_path/$kernel_name-homp.txt"
	echo "homp: $out_pf."
    echo "---------------------------"
	if [ "$kernel_name" == "hj2" ]; then
		out_pf="$out_path/$kernel_name-homp1.txt"
		time taskset -c $smt_core0,$smt_core1 ./$kernel_homp -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 

		out_pf="$out_path/$kernel_name-homp2.txt"
		time taskset -c $smt_core0,$smt_core1 ./$kernel_homp -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 

		out_pf="$out_path/$kernel_name-homp3.txt"
		time taskset -c $smt_core0,$smt_core1 ./$kernel_homp -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 
	elif [ "$kernel_name" == "hj8" ]; then
		out_pf="$out_path/$kernel_name-homp1.txt"
		time taskset -c $smt_core0,$smt_core1 ./$kernel_homp -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 

		out_pf="$out_path/$kernel_name-homp2.txt"
		time taskset -c $smt_core0,$smt_core1 ./$kernel_homp -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 

		out_pf="$out_path/$kernel_name-homp3.txt"
		time taskset -c $smt_core0,$smt_core1 ./$kernel_homp -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 
	else
		out_pf="$out_path/$kernel_name-homp1.txt"
		time taskset -c $smt_core0,$smt_core1 ./$kernel_homp > $out_pf 2>&1

		out_pf="$out_path/$kernel_name-homp2.txt"
		time taskset -c $smt_core0,$smt_core1 ./$kernel_homp > $out_pf 2>&1

		out_pf="$out_path/$kernel_name-homp3.txt"
		time taskset -c $smt_core0,$smt_core1 ./$kernel_homp > $out_pf 2>&1
	fi
fi 

if [ "$exe_swpf" == "1" ]; then
	out_pf="$out_path/$kernel_name-swpf.txt"
	echo "swpf: $out_pf."
    echo "---------------------------"
	if [ "$kernel_name" == "hj2" ]; then
		out_pf="$out_path/$kernel_name-swpf1.txt"
		time taskset -c $smt_core0 ./$kernel_swpf -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 

		out_pf="$out_path/$kernel_name-swpf2.txt"
		time taskset -c $smt_core0 ./$kernel_swpf -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 

		out_pf="$out_path/$kernel_name-swpf3.txt"
		time taskset -c $smt_core0 ./$kernel_swpf -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 
	elif [ "$kernel_name" == "hj8" ]; then
		out_pf="$out_path/$kernel_name-swpf1.txt"
		time taskset -c $smt_core0 ./$kernel_swpf -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 

		out_pf="$out_path/$kernel_name-swpf2.txt"
		time taskset -c $smt_core0 ./$kernel_swpf -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 

		out_pf="$out_path/$kernel_name-swpf3.txt"
		time taskset -c $smt_core0 ./$kernel_swpf -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 
	else
		out_pf="$out_path/$kernel_name-swpf1.txt"
		time taskset -c $smt_core0 ./$kernel_swpf > $out_pf 2>&1

		out_pf="$out_path/$kernel_name-swpf2.txt"
		time taskset -c $smt_core0 ./$kernel_swpf > $out_pf 2>&1

		out_pf="$out_path/$kernel_name-swpf3.txt"
		time taskset -c $smt_core0 ./$kernel_swpf > $out_pf 2>&1
	fi
fi 

if [ "$exe_htpf" == "1" ]; then
	out_pf="$out_path/$kernel_name-htpf.txt"
	echo "htpf: $out_pf."
    echo "---------------------------"
	if [ "$kernel_name" == "hj2" ]; then
		out_pf="$out_path/$kernel_name-htpf1.txt"
		time taskset -c $smt_core0,$smt_core1 ./$kernel_htpf -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 

		out_pf="$out_path/$kernel_name-htpf2.txt"
		time taskset -c $smt_core0,$smt_core1 ./$kernel_htpf -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 

		out_pf="$out_path/$kernel_name-htpf3.txt"
		time taskset -c $smt_core0,$smt_core1 ./$kernel_htpf -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 
	elif [ "$kernel_name" == "hj8" ]; then
		out_pf="$out_path/$kernel_name-htpf1.txt"
		time taskset -c $smt_core0,$smt_core1 ./$kernel_htpf -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 

		out_pf="$out_path/$kernel_name-htpf2.txt"
		time taskset -c $smt_core0,$smt_core1 ./$kernel_htpf -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 

		out_pf="$out_path/$kernel_name-htpf3.txt"
		time taskset -c $smt_core0,$smt_core1 ./$kernel_htpf -a NPO_st -r 12800000 -s 12800000 > $out_pf 2>&1 
	else
		out_pf="$out_path/$kernel_name-htpf1.txt"
		time taskset -c $smt_core0,$smt_core1 ./$kernel_htpf > $out_pf 2>&1

		out_pf="$out_path/$kernel_name-htpf2.txt"
		time taskset -c $smt_core0,$smt_core1 ./$kernel_htpf > $out_pf 2>&1

		out_pf="$out_path/$kernel_name-htpf3.txt"
		time taskset -c $smt_core0,$smt_core1 ./$kernel_htpf > $out_pf 2>&1
	fi
fi

exit
