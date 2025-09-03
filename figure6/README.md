Figure 6 of the paper compares the performance of Ghost Threading with different approaches 
(the baseline, state-of-the-art software prefetching technique, and parallelization technique via SMT) on an idle server. We aim to reproduce the results of the first three bars presented in this figure (i.e., the Ghost Threading compiler results are not included).

# Check core affinity
Before running the experiment, check the core affinity on your system in order to 
pin the two threads from parallelized approach and Ghost Threading to the same physical
core that supports Hyper Threading:
```sh
cat /sys/devices/system/cpu/cpu0/topology/thread_siblings_list
```
On my system the output of this command is:
```sh
0-1 
```
this means that core 0 and 1 are two logic cores of the same physical core that supports
Hyper Threading. 

# Run the experiment
After checking core affinity, use the two logic cores of the same physical core to run 
the code with Hyper Threading. For example, core 0 and core 1 should be used on my system: 
```sh
# the first and second arguments select the two cores used 
./run.sh 0 1
```
This script executes the related binaries and store the results in `figure6.pdf` and `speedup.csv`. The execution 
time of each benchmark can be checked in `time.csv`. On my system this took around 26 hours. 

## (Optional) Speed up the experiment process
To accelerate the experiment process, one optional solution is to reduce the repeat time of 
one long benchmark (`gap.tc`), which takes around 87 minutes when using `kron` input graph 
on my system. As we repeat all benchmarks for three times to compute the average execution time,
this long benchmark causes significant impact to the total experiment time. If `gap.tc` is only executed
once for all its four variants (i.e., baseline, software prefetching, parallelization, and ghost threading),
the experiment time will be reduced to 11 hours on my system. Based on our observations, `gap.tc` exhibits 
very small variations across repeated runs, so reducing its repetition count has minimal impact on result accuracy.

**This is only an optional choice if you do not have enough time to run the whole experiment.**

To adjust the repeat time of `gap.tc`, change the value of `tc_repeat` variable (line 16) in 
`path_to_the_artifact/gap/test.sh`: 
```sh
# in path_to_the_artifact/gap/test.sh
tc_repeat=1 # run gap.tc once instead of three times (default: 3)
```

## (Optional) Adjust the default experiment configurations
Different system configurations can affect the behavior of the same benchmark, which may 
influence both the workloads selected for Ghost Threading and the tuning of inter-thread 
synchronization hyperparameters. The scripts and source code provided in this artifact are 
configured based on my system and should work without modification on similar systems.

In general, no manual adjustments are needed unless you are using a significantly different 
platform. I recommend keeping the default configuration if your system is based on Intel Core 
12th-generation (or newer) processors.

You can adjust the configuration by following the steps below  
(**only do this if the results are totally not comparable to mine using the default configuration**).

### Inter-thread synchronization hyper-parameters
**GAP**: edit the parameters (`syncfreqs`, `skips`, `serialize_thresholds`, and `unserialize_thresholds`) 
from line 40 to 201 in `path_to_the_artifact/gap/test.sh`. 

**HPC**: edit the source code in `path_to_the_artifact/hpc/`. 

### Workload selected by our heuristic
1. Profile all evaluated benchmarks with the train input based on the instructions in 
`path_to_the_artifact/profile/`. 
2. Based on the profiling results, edit the `selected_workloads` list in `path_to_the_artifact/figure6/compute_speedup.py`:
```python
# List of workloads selected by heuristic
selected_workloads = {
    "bc-kron", "bc-twitter", "bc-urand",
    "bfs-kron", "bfs-twitter", "bfs-urand",
    "cc-kron", "cc-twitter", "cc-urand", "cc-web",
    "sssp-kron", "sssp-twitter", "sssp-urand", "sssp-web",
    "tc-urand",
    "camel", "kangaroo", "hj2", "hj8"
}
```
