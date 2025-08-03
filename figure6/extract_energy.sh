#!/usr/bin/bash

outfile="energy.csv"
echo "workload,avg energy-pkg(J)" > $outfile
# echo "workload,time1(s),time2(s),time3(s),avg(s)" > $outfile

methods=("baseline" "homp" "swpf" "htpf")

#------------gap------------
input_dir="../gap/output/figure6"

kernels=(bc bfs cc pr sssp tc)
graphs=(kron twitter urand road web kronU twitterU urandU roadU webU)

for kernel in "${kernels[@]}"; do
  for graph in "${graphs[@]}"; do
    for method in "${methods[@]}"; do
      file="$input_dir/energy-${kernel}-${graph}-${method}.txt"
        if [[ -f "$file" ]]; then
            energy=$(grep -oP '^\s*\K[0-9.,]+(?= Joules)' "$file" | tr -d ',')
            avg=$(awk -v e="$energy" 'BEGIN { printf "%.6f", e / 3 }')
            echo "${kernel}-${graph}-${method},$avg" >> $outfile
        fi
        # done
    done
  done
done

#------------HPC------------
input_dir="../hpc/output/figure6"

#------------camel------------
for method in "${methods[@]}"; do
    total=0
    count=0
    workload=camel
    echo -n "$workload-${method}," >> $outfile
    for file in $input_dir/energy-$workload-${method}*; do
        if [[ -f "$file" ]]; then
            energy=$(grep -oP '^\s*\K[0-9.,]+(?= Joules)' "$file" | tr -d ',')
            if [[ -n $energy ]]; then
                total=$(awk -v t="$total" -v e="$energy" 'BEGIN { printf "%.6f", t + e }')
                ((count++))
            fi
        fi
    done

    if ((count > 0)); then
        avg=$(awk -v t="$total" -v c="$count" 'BEGIN { printf "%.6f", t / c }')
        echo $avg >> $outfile
    else
        echo "NA" >> $outfile
    fi 
done

#------------kangaroo------------
for method in "${methods[@]}"; do
    total=0
    count=0
    workload=kangaroo
    echo -n "$workload-${method}," >> $outfile
    for file in $input_dir/energy-$workload-${method}*; do
        if [[ -f "$file" ]]; then
            energy=$(grep -oP '^\s*\K[0-9.,]+(?= Joules)' "$file" | tr -d ',')
            if [[ -n $energy ]]; then
                total=$(awk -v t="$total" -v e="$energy" 'BEGIN { printf "%.6f", t + e }')
                ((count++))
            fi
        fi
    done

    if ((count > 0)); then
        avg=$(awk -v t="$total" -v c="$count" 'BEGIN { printf "%.6f", t / c }')
        echo $avg >> $outfile
    else
        echo "NA" >> $outfile
    fi 
done

#------------nas-is------------
for method in "${methods[@]}"; do
    total=0
    count=0
    workload=is
    echo -n "$workload-${method}," >> $outfile
    for file in $input_dir/energy-$workload-${method}*; do
        if [[ -f "$file" ]]; then
            energy=$(grep -oP '^\s*\K[0-9.,]+(?= Joules)' "$file" | tr -d ',')
            if [[ -n $energy ]]; then
                total=$(awk -v t="$total" -v e="$energy" 'BEGIN { printf "%.6f", t + e }')
                ((count++))
            fi
        fi
    done

    if ((count > 0)); then
        avg=$(awk -v t="$total" -v c="$count" 'BEGIN { printf "%.6f", t / c }')
        echo $avg >> $outfile
    else
        echo "NA" >> $outfile
    fi 
done

#------------hj2------------
for method in "${methods[@]}"; do
    total=0
    count=0
    workload=hj2
    echo -n "$workload-${method}," >> $outfile
    for file in $input_dir/energy-$workload-${method}*; do
        if [[ -f "$file" ]]; then
            energy=$(grep -oP '^\s*\K[0-9.,]+(?= Joules)' "$file" | tr -d ',')
            if [[ -n $energy ]]; then
                total=$(awk -v t="$total" -v e="$energy" 'BEGIN { printf "%.6f", t + e }')
                ((count++))
            fi
        fi
    done

    if ((count > 0)); then
        avg=$(awk -v t="$total" -v c="$count" 'BEGIN { printf "%.6f", t / c }')
        echo $avg >> $outfile
    else
        echo "NA" >> $outfile
    fi 
done

#------------hj8------------
for method in "${methods[@]}"; do
    total=0
    count=0
    workload=hj8
    echo -n "$workload-${method}," >> $outfile
    for file in $input_dir/energy-$workload-${method}*; do
        if [[ -f "$file" ]]; then
            energy=$(grep -oP '^\s*\K[0-9.,]+(?= Joules)' "$file" | tr -d ',')
            if [[ -n $energy ]]; then
                total=$(awk -v t="$total" -v e="$energy" 'BEGIN { printf "%.6f", t + e }')
                ((count++))
            fi
        fi
    done

    if ((count > 0)); then
        avg=$(awk -v t="$total" -v c="$count" 'BEGIN { printf "%.6f", t / c }')
        echo $avg >> $outfile
    else
        echo "NA" >> $outfile
    fi 
done