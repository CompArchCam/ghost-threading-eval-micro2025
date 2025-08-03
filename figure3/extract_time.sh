#!/usr/bin/bash

outfile="time.csv"
echo "workload,avg time(s)" > $outfile
# echo "workload,time1(s),time2(s),time3(s),avg(s)" > $outfile

methods=("baseline" "homp" "swpf" "htpf")

input_dir="../hpc/output/figure3"

#------------camel------------
for method in "${methods[@]}"; do
    total=0
    count=0
    workload=camel
    echo -n "original-camel-${method}," >> $outfile
    for file in $input_dir/$workload-${method}*; do
        if [[ -f "$file" ]]; then
            time=$(grep -Ei "time\(ms\)|time taken \(ms\)" "$file" | \
                   sed -E 's/.*[Tt]ime *[Tt]aken *\(ms\) *: *([0-9.]+).*/\1/; s/.*[Tt]ime *\(ms\) *: *([0-9.]+).*/\1/')

            if [[ "$time" =~ ^[0-9.]+$ ]]; then
                # echo -n "$time," >> $outfile
                total=$(awk -v t="$total" -v v="$time" 'BEGIN { printf "%.6f", t + v }')
                count=$((count + 1))
            fi
        fi
    done

    if ((count > 0)); then
        avg=$(awk -v t="$total" -v c="$count" 'BEGIN { printf "%.2f", (t / c) / 1000 }')
        echo $avg >> $outfile
    else
        echo "NA" >> $outfile
    fi 
done

#------------camel-paral------------
for method in "${methods[@]}"; do
    total=0
    count=0
    workload=camel-paral
    echo -n "suitable-for-parallelization-${method}," >> $outfile
    for file in $input_dir/$workload-${method}*; do
        if [[ -f "$file" ]]; then
            time=$(grep -Ei "time *\(ms\)" "$file" | sed -nE 's/.*[Tt]ime *\(ms\) *: *([0-9.]+).*/\1/p')

            if [[ "$time" =~ ^[0-9.]+$ ]]; then
                # echo -n "$time," >> $outfile
                total=$(awk -v t="$total" -v v="$time" 'BEGIN { printf "%.6f", t + v }')
                count=$((count + 1))
            fi
        fi
    done

    if ((count > 0)); then
        avg=$(awk -v t="$total" -v c="$count" 'BEGIN { printf "%.2f", (t / c) / 1000 }')
        echo $avg >> $outfile
    else
        echo "NA" >> $outfile
    fi 
done

#------------camel-ghost------------
for method in "${methods[@]}"; do
    total=0
    count=0
    workload=camel-ghost
    echo -n "suitable-for-ghost-threading-${method}," >> $outfile
    for file in $input_dir/$workload-${method}*; do
        if [[ -f "$file" ]]; then
            time=$(grep -Ei "time *\(ms\)" "$file" | sed -nE 's/.*[Tt]ime *\(ms\) *: *([0-9.]+).*/\1/p')

            if [[ "$time" =~ ^[0-9.]+$ ]]; then
                # echo -n "$time," >> $outfile
                total=$(awk -v t="$total" -v v="$time" 'BEGIN { printf "%.6f", t + v }')
                count=$((count + 1))
            fi
        fi
    done

    if ((count > 0)); then
        avg=$(awk -v t="$total" -v c="$count" 'BEGIN { printf "%.2f", (t / c) / 1000 }')
        echo $avg >> $outfile
    else
        echo "NA" >> $outfile
    fi 
done