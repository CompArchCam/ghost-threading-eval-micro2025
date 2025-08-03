This repository contains the source code and scripts needed to run several of 
the experiments from the paper "Ghost Threading: Helper-Thread Prefetching for Real Systems" 
submitted to MICRO2025.

We aim to regenerate results for Figure 3, Figure 5, Figure 6, and Figure 7. Since this technique targets on 
real systems and involoves a lot of system-dependent configurations, we'd expect the results to have 
quite some significant variation across different hardware. Hopefully the main trends of the 
paper will be visible in your output. 

# Requirements

To run these scripts you will need:

1. A physical system with x86-64 Intel processor, which has at least 8 cores and supports Hyper 
Threading and the [`serialize`](https://www.intel.com/content/www/us/en/content-details/825743/intel-64-and-ia-32-architectures-software-developer-s-manual-combined-volumes-1-2a-2b-2c-2d-3a-3b-3c-3d-and-4.html) instruction (ideally Intel Core i7-12700 processor with 96GB memory or similar systems), running Linux (ideally Ubuntu 24.04 or 22.04). 
2. The following dependencies: gcc/g++ (ideally >= v13.0), clang (ideally >= v17.0), and python3.
3. For Figure 6 additional dependencies: perf.
4. For Figure 7: [Intel RDT Software Package](https://github.com/intel/intel-cmt-cat).

# Build
To compile our source code and build input data used for evaluation: 
```sh
./build.sh
```
This process takes around one hour on my system. 

# Running experimental workflows (reproducing figures)
The `README` files in subfolders (`figure3`, `figure5`, `figure6`, and `figure7`) provide instructions of reproducing the corrsponding figures. When reproducing the results, try to
avoid running other applications on the server. 