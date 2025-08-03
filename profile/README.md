# Install OptiWISE
We use `OptiWISE` to profile the evaluted applications. 
Before running our profiling scripts, install the tool by the instructions in 
[OptiWISE repository](https://github.com/CompArchCam/optiwise). 

Check if it is working by running `optiwise check`. It may
report that it can't work because of kernel permissions settings.  Running:
```sh
sudo sysctl -w kernel.perf_event_paranoid=2
```
will allow ordinary users on the system to access performance counters,
which will likely make it work.

# Profile the benchmarks
Run:
```sh
./profile.sh
```
The profiling results will be stored in `path_to_the_artifact/profile/optiwise_result/`. 
On my system this took around 7 hours. 

If you want to use other profiling tools, `exec_all.mk` describes the binaries and input arguments. 