#!/usr/bin/bash

#----------only set these parameters----------
exe_baslines=1
exe_homp=1
exe_swpf=1

exe_pf=1 # disable tpf and htpf 
exe_tpf=0
exe_htpf=1
exe_best=0

out_path=output/figure7

repeat=3
tc_repeat=3 # use this to adjust tc repeat times
#----------only set these parameters----------
kernel_name=$1
graph_name=$2
smt_core0=$3
smt_core1=$4

if [ "$kernel_name" == "sssp" ]; then
    graph_postfix=wsg
else
    graph_postfix=sg
fi 

if [ "$kernel_name" == "bfs" ]; then
    exe_best=$exe_htpf
    exe_htpf=0
fi

kernel_tpf_name="$kernel_name"_tpf
graph=benchmark/graphs/$graph_name.$graph_postfix
graph_macro=$(echo "$graph_name" | tr 'a-z' 'A-Z')
echo $graph_macro
kernel_name_htpf=$kernel_name-$graph_name-htpf

if [ "$kernel_name" == "bfs" ]; then
    if [ "$graph_name" == "kron" ]; then
        syncfreqs=('200')
        skips=('64')
        serialize_thresholds=('300')
        unserialize_thresholds=('10')
    elif [ "$graph_name" == "twitter" ]; then
        syncfreqs=('100')
        skips=('64')
        serialize_thresholds=('300')
        unserialize_thresholds=('30')
    elif [ "$graph_name" == "urand" ]; then
        syncfreqs=('3')
        skips=('7')
        serialize_thresholds=('100')
        unserialize_thresholds=('10')
    elif [ "$graph_name" == "road" ]; then
        syncfreqs=('500')
        skips=('0')
        serialize_thresholds=('800')
        unserialize_thresholds=('30') 
    elif [ "$graph_name" == "web" ]; then
        syncfreqs=('700')
        skips=('128')
        serialize_thresholds=('800')
        unserialize_thresholds=('30')
    fi
elif [ "$kernel_name" == "bc" ]; then
    chunk_size=64
    if [ "$graph_name" == "kron" ]; then
        syncfreqs=('80')
        skips=('32')
        serialize_thresholds=('80')
        unserialize_thresholds=('10')
    elif [ "$graph_name" == "twitter" ]; then
        syncfreqs=('80')
        skips=('32')
        serialize_thresholds=('150')
        unserialize_thresholds=('10')
    elif [ "$graph_name" == "urand" ]; then
        syncfreqs=('2')
        skips=('8')
        serialize_thresholds=('60')
        unserialize_thresholds=('1') 
    elif [ "$graph_name" == "road" ]; then
        syncfreqs=('120')
        skips=('16')
        serialize_thresholds=('1000')
        unserialize_thresholds=('10') 
    elif [ "$graph_name" == "web" ]; then
        syncfreqs=('200')
        skips=('64')
        serialize_thresholds=('600')
        unserialize_thresholds=('10')
    fi
elif [ "$kernel_name" == "cc" ]; then
    chunk_size=16384
    if [ "$graph_name" == "kron" ]; then
        syncfreqs=('800')
        skips=('400')
        serialize_thresholds=('800')
        unserialize_thresholds=('50')
    elif [ "$graph_name" == "twitter" ]; then
        syncfreqs=('800')
        skips=('400' )
        serialize_thresholds=('600')
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
        syncfreqs=('800')
        skips=('90')
        serialize_thresholds=('150')
        unserialize_thresholds=('50')
    fi
elif [ "$kernel_name" == "sssp" ]; then
    if [ "$graph_name" == "kron" ]; then
        syncfreqs=('600')
        skips=('32')
        serialize_thresholds=('120')
        unserialize_thresholds=('10')
    elif [ "$graph_name" == "twitter" ]; then
        syncfreqs=('200')
        skips=('16')
        serialize_thresholds=('800')
        unserialize_thresholds=('10')
    elif [ "$graph_name" == "urand" ]; then
        syncfreqs=('7')
        skips=('21')
        serialize_thresholds=('60')
        unserialize_thresholds=('11')
    elif [ "$graph_name" == "web" ]; then
        syncfreqs=('15')
        skips=('60')
        serialize_thresholds=('160')
        unserialize_thresholds=('8')
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
    repeat=$tc_repeat
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

if [ "$exe_baslines" == "1" ]; then
	g++ -std=c++11 -pthread -O3 -Wall -w src/$kernel_name.cc -o $kernel_name
	out_pf="$out_path/$kernel_name-$graph_name-baseline.txt"
	echo "Baseline: $out_pf."
	taskset -c $smt_core0 ./$kernel_name -f $graph -n $repeat > $out_pf 2>&1
	rm $kernel_name
fi 

if [ "$exe_homp" == "1" ]; then
	g++ -std=c++11 -pthread -fopenmp -O3 -Wall -w -DOMP -DNT=2 src/$kernel_name.cc -o $kernel_name-omp
	out_pf="$out_path/$kernel_name-$graph_name-homp.txt"
	echo "HOMP: $out_pf."
	taskset -c $smt_core0,$smt_core1 ./$kernel_name-omp -f $graph -n $repeat > $out_pf 2>&1
	rm $kernel_name-omp
fi 

if [ "$exe_swpf" == "1" ]; then
	g++ -std=c++11 -pthread -O3 -Wall -w -DSWPF src/$kernel_name.cc -o $kernel_name-swpf
	out_pf="$out_path/$kernel_name-$graph_name-swpf.txt"
	echo "SWPF: $out_pf."
	taskset -c $smt_core0 ./$kernel_name-swpf -f $graph -n $repeat > $out_pf 2>&1
	rm $kernel_name-swpf
fi 

if [ "$exe_pf" == "1" ]; then
g++ -std=c++11 -pthread -O3 -Wall -w -DMEMBW -DTUNING -DHTPF -D$graph_macro src/$kernel_tpf_name.cc -o $kernel_tpf_name
for syncfreq in "${syncfreqs[@]}"
do
	for skip in "${skips[@]}"
	do
		for threshold in "${serialize_thresholds[@]}"
		do
			for un_threshold in "${unserialize_thresholds[@]}"
			do
				#htpf 
				if [ "$exe_htpf" == "1" ]; then
				if [ $un_threshold -le $threshold ]; then
					out_pf="$out_path/$kernel_name_htpf.txt"
					echo "HTPF: $out_pf."
					taskset -c $smt_core0,$smt_core1 ./$kernel_tpf_name -f $graph -n $repeat -p $syncfreq -o $threshold -j $skip -q $un_threshold > $out_pf 2>&1
				fi 
				fi
			done 
		done
	done 
done
rm $kernel_tpf_name
fi

if [ "$exe_best" == "1" ]; then
g++ -std=c++11 -fopenmp -pthread -O3 -Wall -w -DMEMBW -DOMP -DBEST -DTUNING -DHTPF -D$graph_macro src/$kernel_tpf_name.cc -o $kernel_tpf_name
for syncfreq in "${syncfreqs[@]}"
do
	for skip in "${skips[@]}"
	do
		for threshold in "${serialize_thresholds[@]}"
		do
			for un_threshold in "${unserialize_thresholds[@]}"
			do
				if [ $un_threshold -le $threshold ]; then
					out_pf="$out_path/$kernel_name_htpf.txt"
					echo "TPF: $out_pf."
					taskset -c $smt_core0,$smt_core1 ./$kernel_tpf_name -f $graph -n $repeat -p $syncfreq -o $threshold -j $skip -q $un_threshold > $out_pf 2>&1
				fi 
			done 
		done
	done 
done
rm $kernel_tpf_name
fi

exit
