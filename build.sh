#!/usr/bin/bash

#-----------build graphs for gap-----------
cd gap
echo "Building graphs..."
make converter && make bench-graphs &&
echo "Successfully build graphs."

#-----------build HPC workloads-----------
# camel 
cd ../hpc/camel
./my_compile.sh && echo "Successfully build camel."

# kangaroo 
cd ../kangaroo
./my_compile.sh && echo "Successfully build kangaroo."

# hj2 
cd ../hashjoin-ph-2
./my_compile.sh && echo "Successfully build hj2."

# hj8 
cd ../hashjoin-ph-8
./my_compile.sh && echo "Successfully build hj8."

# nas-is 
cd ../nas-is
./my_compile.sh && echo "Successfully build nas-is." &&

# camel-nested 
cd ../camel-nested
./my_compile.sh && echo "Successfully build camel-nested."

echo "Build complete."