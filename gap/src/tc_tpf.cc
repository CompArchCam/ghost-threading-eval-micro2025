// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

// Encourage use of gcc's parallel algorithms (for sort for relabeling)
#ifdef _OPENMP
  #define _GLIBCXX_PARALLEL
#endif

#include <algorithm>
#include <cinttypes>
#include <iostream>
#include <vector>
#include <thread>

#include "omp.h" 

#include "benchmark.h"
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "pvector.h"
#include "pf_support.h"

/*
GAP Benchmark Suite
Kernel: Triangle Counting (TC)
Author: Scott Beamer

Will count the number of triangles (cliques of size 3)

Input graph requirements:
  - undirected
  - has no duplicate edges (or else will be counted as multiple triangles)
  - neighborhoods are sorted by vertex identifiers

Other than symmetrizing, the rest of the requirements are done by SquishCSR
during graph building.

This implementation reduces the search space by counting each triangle only
once. A naive implementation will count the same triangle six times because
each of the three vertices (u, v, w) will count it in both ways. To count
a triangle only once, this implementation only counts a triangle if u > v > w.
Once the remaining unexamined neighbors identifiers get too big, it can break
out of the loop, but this requires that the neighbors are sorted.

This implementation relabels the vertices by degree. This optimization is
beneficial if the average degree is sufficiently high and if the degree
distribution is sufficiently non-uniform. To decide whether to relabel the
graph, we use the heuristic in WorthRelabelling.
*/

#define ORDER_READ memory_order_relaxed
#define ORDER_WRITE memory_order_relaxed

// #define HTPF
// #define OMP
// #define TIME

// #define TWITTER
// #define INNER_MOST
// #define TUNING

#if defined(KRON) || defined(TWITTERU) //|| defined(WEBU)
#define INNER
#endif 

#ifdef TIME
#define TOTAL_ITER 27197768325
#define FREQ 1000
uint32_t stamp_counter, array_counter; 
std::chrono::_V2::system_clock::time_point time_array[TOTAL_ITER/FREQ+100]; 

#if defined(HTPF)
  #if defined(KRON)
    #define OUTPUT "timestamp/tc.htpf.kron.csv"
  #elif defined(TWITTER)
    #define OUTPUT "timestamp/tc.htpf.twitter.csv"
  #elif defined(URAND)
    #define OUTPUT "timestamp/tc.htpf.urand.csv"
  #elif defined(ROAD)
    #define OUTPUT "timestamp/tc.htpf.road.csv"
  #elif defined(WEB)
    #define OUTPUT "timestamp/tc.htpf.web.csv"
  #endif 
#elif defined(OMP)
  #if defined(KRON)
    #define OUTPUT "timestamp/tc.homp.kron.csv"
  #elif defined(TWITTER)
    #define OUTPUT "timestamp/tc.homp.twitter.csv"
  #elif defined(URAND)
    #define OUTPUT "timestamp/tc.homp.urand.csv"
  #elif defined(ROAD)
    #define OUTPUT "timestamp/tc.homp.road.csv"
  #elif defined(WEB)
    #define OUTPUT "timestamp/tc.homp.web.csv"
  #endif 
#else
  #if defined(KRON)
    #define OUTPUT "timestamp/tc.kron.csv"
  #elif defined(TWITTER)
    #define OUTPUT "timestamp/tc.twitter.csv"
  #elif defined(URAND)
    #define OUTPUT "timestamp/tc.urand.csv"
  #elif defined(ROAD)
    #define OUTPUT "timestamp/tc.road.csv"
  #elif defined(WEB)
    #define OUTPUT "timestamp/tc.web.csv"
  #endif 
#endif 
#endif 

using namespace std;

TimeDiff time_diff; 
HyperParam_PfT hyper_param; 

void PrefetchThread(const Graph *g) {
  #if defined(URAND) && !defined(TUNING)
  HyperParam_PfT hyperparam = {.sync_frequency = 1, .skip_offset = 7, 
                               .serialize_threshold = 23, .unserialize_threshold = 15}; 
  #elif defined(ROADU) && !defined(TUNING)
  HyperParam_PfT hyperparam = {.sync_frequency = 8, .skip_offset = 7, 
                               .serialize_threshold = 23, .unserialize_threshold = 18}; 
  #else
  HyperParam_PfT hyperparam = hyper_param; 
  #endif 
  bool serialize_flag = false; 
  for (NodeID u=0; u < g->num_nodes(); u++) { 
    for (NodeID v : g->out_neigh(u)) { 
      if (serialize_flag) 
          asm volatile ("serialize\n\t"); 
      if (v > u)
        break;
      g->out_neigh(v).prefetch_begin(); 
      auto it = g->out_neigh(v).begin(); 
      for (NodeID w : g->out_neigh(u)) { 
        if (serialize_flag) 
            asm volatile ("serialize\n\t"); 
        if (w > v)
          break;
        __builtin_prefetch(it); 
      }
    }
    if (serialize_flag) 
      asm volatile ("serialize\n\t"); 
    sync<NodeID>(u, 1, u, false, time_diff, ORDER_READ, serialize_flag, hyperparam); 
  }
}

void PrefetchThread_inner(const Graph *g) { // for kron, twitter, and web (although not mem-intensive)
  #if defined(KRONU) && !defined(TUNING)
  HyperParam_PfT hyperparam = {.sync_frequency = 1, .skip_offset = 3, 
                               .serialize_threshold = 13, .unserialize_threshold = 8}; 
  #elif defined(TWITTERU) && !defined(TUNING)
  HyperParam_PfT hyperparam = {.sync_frequency = 30, .skip_offset = 1, 
                               .serialize_threshold = 1000, .unserialize_threshold = 980}; 
  #else
  HyperParam_PfT hyperparam = hyper_param; 
  #endif 
  bool serialize_flag = false; 
  bool prefetch = true; 
  size_t j = 0; 
  for (NodeID u=0; u < g->num_nodes(); u++) { 
    for (NodeID v : g->out_neigh(u)) { 
      if (serialize_flag) 
          asm volatile ("serialize\n\t"); 
      if (v > u)
        break;
      g->out_neigh(v).prefetch_begin(); 
      // #ifdef INNER_MOST
      auto it = g->out_neigh(v).begin(); 
      for (NodeID w : g->out_neigh(u)) { 
        if (serialize_flag) 
            asm volatile ("serialize\n\t"); 
        if (w > v)
          break;
        if (prefetch)
          __builtin_prefetch(it); 
        /*-----inner_most sync-----*/
        j++; 
        if (j % hyper_param.sync_frequency == 0 || serialize_flag || (!prefetch)) {
          size_t main_j = time_diff.read_atomic_main(ORDER_READ); 
          // time_diff.insert_into_atomic_histogram(main_j, j); 
          if (main_j >= j) {
            serialize_flag = false; 
            prefetch = false; 
          } else if (j - main_j > hyper_param.serialize_threshold) {
            serialize_flag = true; 
            prefetch = true; 
          } else if (j - main_j < hyper_param.unserialize_threshold) {
            serialize_flag = false;
            prefetch = true;  
          } else {
            prefetch = true; 
          }
        }
        /*-----inner_most sync-----*/
      }
    }
  }
}

size_t OrderedCount(const Graph &g) {
  size_t total = 0;
  #if defined(HTPF) && !defined(INNER)
  thread PF(PrefetchThread, &g); 
  #elif defined(HTPF) && defined(INNER)
  thread PF(PrefetchThread_inner, &g); 
  time_diff.set_atomic_main(0, ORDER_WRITE); 
  #endif 
  #pragma omp parallel for reduction(+ : total) schedule(dynamic, 64)
  for (NodeID u=0; u < g.num_nodes(); u++) { 
    #if defined(HTPF) && !defined(INNER)
    time_diff.set_atomic_main(u, ORDER_WRITE); 
    #endif 
    for (NodeID v : g.out_neigh(u)) {
      if (v > u)
        break;
      auto it = g.out_neigh(v).begin(); 
      for (NodeID w : g.out_neigh(u)) {
        #ifdef TIME
        #ifdef OMP
        if (omp_get_thread_num() == 0) {
        #endif // OMP
          if (stamp_counter % FREQ == 0) {
            time_array[array_counter] = chrono::high_resolution_clock::now(); 
            array_counter+=1; 
          }
          stamp_counter+=1; 
        #ifdef OMP
        }
        #endif // OMP 
        #endif // TIME
        if (w > v)
          break;
        while (*it < w) 
          it++;
        if (w == *it)
          total++;
        #if defined(HTPF) && defined(INNER)
        time_diff.add_atomic_main(1, ORDER_WRITE); // inner most sync 
        #endif 
      }
    }
  }
  #ifdef HTPF
  PF.join(); 
  #endif 
  return total;
}


// Heuristic to see if sufficiently dense power-law graph
bool WorthRelabelling(const Graph &g) {
  int64_t average_degree = g.num_edges() / g.num_nodes();
  if (average_degree < 10)
    return false;
  SourcePicker<Graph> sp(g);
  int64_t num_samples = min(int64_t(1000), g.num_nodes());
  int64_t sample_total = 0;
  pvector<int64_t> samples(num_samples);
  for (int64_t trial=0; trial < num_samples; trial++) {
    samples[trial] = g.out_degree(sp.PickNext());
    sample_total += samples[trial];
  }
  sort(samples.begin(), samples.end());
  double sample_average = static_cast<double>(sample_total) / num_samples;
  double sample_median = samples[num_samples/2];
  return sample_average / 1.3 > sample_median;
}


// Uses heuristic to see if worth relabeling
size_t Hybrid(const Graph &g) {
  if (WorthRelabelling(g))
    return OrderedCount(Builder::RelabelByDegree(g));
  else
    return OrderedCount(g);
}


void PrintTriangleStats(const Graph &g, size_t total_triangles) {
  cout << total_triangles << " triangles" << endl;
}


// Compares with simple serial implementation that uses std::set_intersection
bool TCVerifier(const Graph &g, size_t test_total) {
  size_t total = 0;
  vector<NodeID> intersection;
  intersection.reserve(g.num_nodes());
  for (NodeID u : g.vertices()) {
    for (NodeID v : g.out_neigh(u)) {
      auto new_end = set_intersection(g.out_neigh(u).begin(),
                                      g.out_neigh(u).end(),
                                      g.out_neigh(v).begin(),
                                      g.out_neigh(v).end(),
                                      intersection.begin());
      intersection.resize(new_end - intersection.begin());
      total += intersection.size();
    }
  }
  total = total / 6;  // each triangle was counted 6 times
  if (total != test_total)
    cout << total << " != " << test_total << endl;
  return total == test_total;
}


int main(int argc, char* argv[]) {
  #if defined(HTPF) 
  cout << "HTPF enabled. "; 
  #if defined(INNER) 
  cout << "INNER sync enabled.\n"; 
  #endif 
  #endif 
  #ifdef OMP
  omp_set_num_threads(2); 
  #endif

  #ifdef TIME
  stamp_counter = 0; 
  array_counter = 0; 
  #endif 

  time_diff.init_atomic(); 
  // time_diff.init_atomic_histogram(1528870792);
  CLApp cli(argc, argv, "triangle count");
  if (!cli.ParseArgs())
    return -1;
  /*-------set hyper parameters for inter-thread sync-------*/
  hyper_param.sync_frequency = cli.sync_frequency(); 
  hyper_param.skip_offset = cli.skip_offset(); 
  hyper_param.serialize_threshold = cli.serialize_threshold(); 
  hyper_param.unserialize_threshold = cli.serialize_threshold() > cli.unserialize_threshold() ? 
            cli.serialize_threshold() - cli.unserialize_threshold() : 0; 
  cout << "sync frequency = " << hyper_param.sync_frequency << ", serialize threshold = " << hyper_param.serialize_threshold 
       << ", unserialize threshold = " << hyper_param.unserialize_threshold << ", skip offset = " << hyper_param.skip_offset << endl; 
  /*-------set hyper parameters for inter-thread sync-------*/
  Builder b(cli);
  Graph g = b.MakeGraph();
  if (g.directed()) {
    cout << "Input graph is directed but tc requires undirected" << endl;
    return -2;
  }
  #ifdef TIME
  auto kernel_start = chrono::high_resolution_clock::now();
  #endif 
  BenchmarkKernel(cli, g, Hybrid, PrintTriangleStats, TCVerifier);
  // time_diff.print_atomic_histogram(); 
  #ifdef TIME
  ofstream myout; 
  myout.open(OUTPUT); 
  for (uint32_t i = 0; i < array_counter; i++) {
    // time_diff[i] = chrono::duration_cast<chrono::nanoseconds>(time_array[i] - kernel_start).count(); 
    myout << chrono::duration_cast<chrono::microseconds>(time_array[i] - kernel_start).count() << endl; 
  }
  #endif 
  return 0;
}
