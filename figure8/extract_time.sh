#!/usr/bin/bash

outfile="time.csv"
echo "workload,avg time(s)" > $outfile
# echo "workload,time1(s),time2(s),time3(s),avg(s)" > $outfile

methods=("baseline" "homp" "swpf" "htpf")

#------------gap------------
input_dir="../gap/output/figure8"

kernels=(bc bfs cc pr sssp tc)
graphs=(kron twitter twitterU urand road roadU web webU)

for kernel in "${kernels[@]}"; do
  for graph in "${graphs[@]}"; do
    for method in "${methods[@]}"; do
      file="$input_dir/${kernel}-${graph}-${method}.txt"
        if [[ -f "$file" ]]; then
            time=$(awk '/^Average Time:/ { print $3 }' "$file")
            echo "${kernel}-${graph}-${method},$time" >> $outfile
        fi
        # done
    done
  done
done

#------------HPC------------
input_dir="../hpc/output/figure8"

#------------camel------------
for method in "${methods[@]}"; do
    total=0
    count=0
    workload=camel
    echo -n "$workload-${method}," >> $outfile
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

#------------kangaroo------------
for method in "${methods[@]}"; do
    total=0
    count=0
    workload=kangaroo
    echo -n "$workload-${method}," >> $outfile
    for file in $input_dir/$workload-${method}*; do
        if [[ -f "$file" ]]; then
            time=$(awk '/time :/ {print $3}' "$file")

            if [[ "$time" =~ ^[0-9.]+$ ]]; then
                # echo -n "$time," >> $outfile
                total=$(awk -v t="$total" -v v="$time" 'BEGIN { printf "%.6f", t + v }')
                count=$((count + 1))
            fi
        fi
    done

    if ((count > 0)); then
        avg=$(awk -v t="$total" -v c="$count" 'BEGIN { printf "%.2f", t / c }')
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
    for file in $input_dir/$workload-${method}*; do
        if [[ -f "$file" ]]; then
            time=$(awk '/time :/ {print $3}' "$file")

            if [[ "$time" =~ ^[0-9.]+$ ]]; then
                # echo -n "$time," >> $outfile
                total=$(awk -v t="$total" -v v="$time" 'BEGIN { printf "%.6f", t + v }')
                count=$((count + 1))
            fi
        fi
    done

    if ((count > 0)); then
        avg=$(awk -v t="$total" -v c="$count" 'BEGIN { printf "%.2f", t / c }')
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
    for file in $input_dir/$workload-${method}*; do
        if [[ -f "$file" ]]; then
            time=$(awk '/my time/ { print $4 }' "$file")

            if [[ -n "$time" ]]; then
                # echo -n "$time," >> $outfile
                total=$(awk -v t="$total" -v v="$time" 'BEGIN { printf "%.6f", t + v }')
                count=$((count + 1))
            fi
        fi
    done

    if ((count > 0)); then
        avg=$(awk -v t="$total" -v c="$count" 'BEGIN { printf "%.2f", t / c }')
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
    for file in $input_dir/$workload-${method}*; do
        if [[ -f "$file" ]]; then
            time=$(awk '/my time/ { print $4 }' "$file")

            if [[ -n "$time" ]]; then
                # echo -n "$time," >> $outfile
                total=$(awk -v t="$total" -v v="$time" 'BEGIN { printf "%.6f", t + v }')
                count=$((count + 1))
            fi
        fi
    done

    if ((count > 0)); then
        avg=$(awk -v t="$total" -v c="$count" 'BEGIN { printf "%.2f", t / c }')
        echo $avg >> $outfile
    else
        echo "NA" >> $outfile
    fi 
done
