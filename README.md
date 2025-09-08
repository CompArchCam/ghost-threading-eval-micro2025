This repository contains the source code and scripts needed to run several of the experiments from the paper:

Ghost Threading: Helper-Thread Prefetching for Real Systems\
Yuxin Guo, Alexandra W. Chadwick, Márton Erdős,, Utpal Bora, Akshay Bhosale, Giacomo Gabrielli and Timothy M. Jones\
International Symposium on Microarchitecture (MICRO)\
October 2025

Please cite this paper if you produce any work that uses this repository.

We aim to regenerate results for Figure 3 and the first three bars of Figures 6 to 8. Since this technique targets on real systems and involoves a lot of system-dependent configurations, we'd expect the results to have quite some significant variation across different hardware. Hopefully the main trends of the paper will be visible in your output.

# Requirements

To run these scripts you will need:

1. A physical system with x86-64 Intel processor, which has at least 8 cores and supports Hyper 
Threading and the [`serialize`](https://www.intel.com/content/www/us/en/content-details/825743/intel-64-and-ia-32-architectures-software-developer-s-manual-combined-volumes-1-2a-2b-2c-2d-3a-3b-3c-3d-and-4.html) instruction (ideally Intel Core i7-12700 processor with 94GB memory or similar systems), running Linux (ideally Ubuntu 24.04 or 22.04). 
2. At least 94GB memory is needed to build the input graphs for some benchmarks. 
3. Around 290GB disk space is needed (mainly for the input graphs). 
4. The following dependencies: gcc/g++ (ideally >= v13.0), clang (ideally >= v17.0), python3, and gnuplot.
Install the software dependencies on Ubuntu:
```sh
sudo apt install gcc g++ clang python3 gnuplot
```
5. For Figure 7 additional dependencies: perf. perf is a part of the Linux kernel and generally does not need installation. 
If perf does not exist on the system, it is available in the `linux-tools-generic` package on Ubuntu:
```sh
sudo apt install linux-tools-generic
```
6. For Figure 8: [Intel RDT Software Package](https://github.com/intel/intel-cmt-cat). To install:
```sh
git clone https://github.com/intel/intel-cmt-cat.git
cd intel-cmt-cat

# Check installation instructions from INSTALL file
make
sudo make install

# Test installation
membw # this shows the usage instructions
```

# Build
To compile our source code and build input data used for evaluation: 
```sh
./build.sh
```
This process takes around one hour on my system. 

# Running experimental workflows (reproducing figures)
The `README` files in subfolders (`figure3`, `figure6`, `figure7`, and `figure8`) provide instructions for reproducing the corresponding figures. When reproducing the results, try to avoid running other applications on the server. 
