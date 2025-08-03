#!/bin/bash

#-----------------------------------
exe_basline=1
exe_homp=1
exe_swpf=1
exe_htpf=1

chunk_size=64

kernel_name=$1
graph_name=$2
thread_num=$3

thread_num2=$(( thread_num * 2))
end_core=$(( thread_num * 2 - 1))

base_cpu_list=$(seq -s, 0 2 $end_core)
homp_cpu_list=$(seq -s, 0 1 $end_core)

repeat=1
#------------------------------------

echo "----------------------------"
echo "thread num: $thread_num"
echo "base list: $base_cpu_list"
echo "homp list: $homp_cpu_list"

# out_path=output/$kernel_name-dynamic-$chunk_size-paral$thread_num
out_path=output/para-sim/$kernel_name-paral$thread_num

if [ "$kernel_name" == "sssp" ]; then
    graph_postfix=wsg
else
    graph_postfix=sg
fi 

kernel_tpf_name="$kernel_name"_tpf_paral
graph=benchmark/graphs/$graph_name-sim.$graph_postfix
graph_macro=$(echo "$graph_name" | tr 'a-z' 'A-Z')

echo "tested graph: $graph_macro"

if [ "$kernel_name" == "bfs" ]; then
    if [ "$graph_name" == "kron" ]; then
        syncfreqs=('200' '300')
        skips=('64' '32')
        serialize_thresholds=('300' '600')
        unserialize_thresholds=('10')
    elif [ "$graph_name" == "twitter" ]; then
        syncfreqs=('100' '200')
        skips=('64' '32')
        serialize_thresholds=('300' '600')
        unserialize_thresholds=('30')
    elif [ "$graph_name" == "urand" ]; then
        syncfreqs=('3' '8')
        skips=('7' '14')
        serialize_thresholds=('100' '300')
        unserialize_thresholds=('10')
    elif [ "$graph_name" == "road" ]; then
        # syncfreqs=('500')
        # skips=('64')
        # serialize_thresholds=('800')
        # unserialize_thresholds=('30') 
        syncfreqs=('120' '300')
        skips=('16' '32')
        serialize_thresholds=('1000' '600')
        unserialize_thresholds=('10') 
    elif [ "$graph_name" == "web" ]; then
        syncfreqs=('700')
        skips=('128' '64')
        serialize_thresholds=('800' '500')
        unserialize_thresholds=('30')
    fi
elif [ "$kernel_name" == "bc" ]; then
    chunk_size=64
    if [ "$graph_name" == "kron" ]; then
        syncfreqs=('80' '160')
        skips=('128' '64')
        serialize_thresholds=('800' '400')
        unserialize_thresholds=('10')
    elif [ "$graph_name" == "twitter" ]; then
        syncfreqs=('80' '120')
        skips=('64' '32')
        serialize_thresholds=('800' '400')
        unserialize_thresholds=('30')
    elif [ "$graph_name" == "urand" ]; then
        syncfreqs=('2')
        skips=('8')
        serialize_thresholds=('60')
        unserialize_thresholds=('1') 
    elif [ "$graph_name" == "road" ]; then
        # syncfreqs=('120')
        # skips=('0')
        # serialize_thresholds=('1500')
        # unserialize_thresholds=('10') 
        syncfreqs=('200' '500')
        skips=('0')
        serialize_thresholds=('80' '200' '400' '800')
        unserialize_thresholds=('30') 
    elif [ "$graph_name" == "web" ]; then
        syncfreqs=('200' '300')
        skips=('0' '16' '32' '64')
        serialize_thresholds=('300' '600' '900')
        unserialize_thresholds=('10')
    fi
elif [ "$kernel_name" == "cc" ]; then
    chunk_size=16384
    if [ "$graph_name" == "kron" ]; then
        syncfreqs=('800' '1000')
        skips=('400' '100')
        serialize_thresholds=('800' '600')
        unserialize_thresholds=('50')
    elif [ "$graph_name" == "twitter" ]; then
        syncfreqs=('800' '400')
        skips=('400' '100')
        serialize_thresholds=('600' '300')
        unserialize_thresholds=('50')
    elif [ "$graph_name" == "urand" ]; then
        syncfreqs=('18')
        skips=('15')
        serialize_thresholds=('18')
        unserialize_thresholds=('3')
    elif [ "$graph_name" == "road" ]; then
        syncfreqs=('800')
        skips=('400')
        serialize_thresholds=('600')
        unserialize_thresholds=('50')
    elif [ "$graph_name" == "web" ]; then
        syncfreqs=('800' '1000')
        skips=('90' '130')
        serialize_thresholds=('150' '300')
        unserialize_thresholds=('50')
    fi
elif [ "$kernel_name" == "sssp" ]; then
    if [ "$graph_name" == "kron" ]; then
        syncfreqs=('800' '500')
        skips=('64' '32')
        serialize_thresholds=('150' '80' '300')
        unserialize_thresholds=('10')
    elif [ "$graph_name" == "twitter" ]; then
        syncfreqs=('1000' '800')
        skips=('32' '64')
        serialize_thresholds=('120' '250' '70')
        unserialize_thresholds=('10')
    elif [ "$graph_name" == "urand" ]; then
        syncfreqs=('7')
        skips=('21')
        serialize_thresholds=('60')
        unserialize_thresholds=('11')
    elif [ "$graph_name" == "web" ]; then
        syncfreqs=('13' '20')
        skips=('18' '30')
        serialize_thresholds=('32' '80')
        unserialize_thresholds=('7')
    fi
elif [ "$kernel_name" == "pr" ]; then
    if [ "$graph_name" == "kron" ]; then
        syncfreqs=('20')
        skips=('30')
        serialize_thresholds=('140')
        unserialize_thresholds=('5')
    elif [ "$graph_name" == "twitter" ]; then
        syncfreqs=('10')
        skips=('30')
        serialize_thresholds=('140')
        unserialize_thresholds=('15')
    elif [ "$graph_name" == "urand" ]; then
        syncfreqs=('20')
        skips=('30')
        serialize_thresholds=('100')
        unserialize_thresholds=('5')
    elif [ "$graph_name" == "road" ]; then
        syncfreqs=('20')
        skips=('60')
        serialize_thresholds=('100')
        unserialize_thresholds=('5')
    elif [ "$graph_name" == "web" ]; then
        syncfreqs=('10')
        skips=('30')
        serialize_thresholds=('100')
        unserialize_thresholds=('5')
    fi
elif [ "$kernel_name" == "tc" ]; then
    chunk_size=64
    if [ "$graph_name" == "kron" ]; then
        syncfreqs=('60')
        skips=('20')
        serialize_thresholds=('800')
        unserialize_thresholds=('50')
    elif [ "$graph_name" == "twitterU" ]; then
        syncfreqs=('30')
        skips=('11')
        serialize_thresholds=('800')
        unserialize_thresholds=('50')
    elif [ "$graph_name" == "urand" ]; then
        syncfreqs=('1')
        skips=('3')
        serialize_thresholds=('13')
        unserialize_thresholds=('5')
    elif [ "$graph_name" == "roadU" ]; then
        syncfreqs=('60')
        skips=('20')
        serialize_thresholds=('800')
        unserialize_thresholds=('50')
    elif [ "$graph_name" == "webU" ]; then
        syncfreqs=('20')
        skips=('7')
        serialize_thresholds=('80')
        unserialize_thresholds=('8')
    fi
fi

mkdir -p $out_path

# baseline 
if [ "$exe_basline" == "1" ]; then
    g++ -std=c++11 -fopenmp -O3 -Wall -w -DOMP -DNT=$thread_num src/$kernel_name.cc -o $kernel_name
    out_pf="$out_path/$kernel_name-$graph_name-baseline.txt"
    echo "Baseline: $out_pf."
    taskset -c $base_cpu_list ./$kernel_name -f $graph -n $repeat > $out_pf 2>&1 
    rm $kernel_name
fi

# homp 
if [ "$exe_homp" == "1" ]; then
    g++ -std=c++11 -fopenmp -O3 -Wall -w -DOMP -DNT=$thread_num2 src/$kernel_name.cc -o $kernel_name-omp
    out_pf="$out_path/$kernel_name-$graph_name-homp.txt"
    echo "HOMP: $out_pf."
    taskset -c $homp_cpu_list ./$kernel_name-omp -f $graph -n $repeat > $out_pf 2>&1 
    rm $kernel_name-omp
fi

# swpf
if [ "$exe_swpf" == "1" ]; then
	g++ -std=c++11 -fopenmp -O3 -Wall -w -DOMP -DNT=$thread_num -DSWPF src/$kernel_name.cc -o $kernel_name-swpf
	out_pf="$out_path/$kernel_name-$graph_name-swpf.txt"
	echo "SWPF: $out_pf."
	taskset -c $base_cpu_list ./$kernel_name-swpf -f $graph -n $repeat > $out_pf 2>&1 
    rm $kernel_name-swpf
fi 

# htpf 
if [ "$exe_htpf" == "1" ]; then
g++ -std=c++11 -fopenmp -pthread -O3 -Wall -w -DCHUNKSIZE=$chunk_size -DTUNING -DHTPF -D$graph_macro -DOMP -DNT=$thread_num src/$kernel_tpf_name.cc -o $kernel_tpf_name
for syncfreq in "${syncfreqs[@]}"
do
	for skip in "${skips[@]}"
	do
		for threshold in "${serialize_thresholds[@]}"
		do
			for un_threshold in "${unserialize_thresholds[@]}"
			do
				if [ $un_threshold -le $threshold ]; then
					out_pf="$out_path/$kernel_name-$graph_name-htpf-$syncfreq-$skip-$threshold-$un_threshold.txt"
					echo "HTPF: $out_pf."
					taskset -c $homp_cpu_list ./$kernel_tpf_name -f $graph -n $repeat -p $syncfreq -o $threshold -j $skip -q $un_threshold > $out_pf 2>&1 
                fi
			done 
		done
	done 
done
rm $kernel_tpf_name
fi
