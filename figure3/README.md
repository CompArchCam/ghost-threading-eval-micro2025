Figure 3 of the paper compares different forms of a synthetic workload called `camel` 
to illustrate different situations that are favorable to different approaches (the baseline, 
state-of-the-art software prefetching technique, parallelization technique via SMT, and Ghost 
Threading), giving a motivational study. 

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
This script executes the related binaries and store the results in `figure3.pdf` and `speedup.csv`. The execution 
time of each benchmark can be checked in `time.csv`. On my system this took around 40 seconds. 