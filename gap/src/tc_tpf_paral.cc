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

#include <sched.h>
#include <pthread.h>

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

#ifndef NT
#define NT 2
#endif 

#ifndef CHUNKSIZE
#define CHUNKSIZE 64
#endif 

// #define HTPF
// #define OMP
// #define TIME

// #define KRON
// #define INNER_MOST
// #define TUNING

#if defined(KRON) || defined(TWITTERU)
#define INNER
#endif 

using namespace std;

OMPSyncAtomic time_diff(NT); 
OMPSyncAtomic end_flag(NT); 
HyperParam_PfT hyper_param; 

void PrefetchThread(int me, const Graph *g, NodeID start) {
  /*-----pin the pf thread to specfic core-----*/ 
  pthread_t self = pthread_self();
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  // CPU_SET(me+64, &cpuset); // dublin 
  CPU_SET((me*2)+1, &cpuset); // beijing 
    
  int rc = pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpuset);
  if (rc != 0)
  {
    printf("Failed to pin pf thread.\n");
    exit(1);
  }
  /*-----pin the pf thread to specfic core-----*/

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
  NodeID local_counter = start; 
  start = time_diff.read(me, ORDER_READ); 
  for (NodeID u = start; u < g->num_nodes(); u++) { 
    local_counter = u - start; 
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
    sync<NodeID>(u, 1, u, false, time_diff, me, ORDER_READ, serialize_flag, hyperparam); 
    if (local_counter >= CHUNKSIZE) {
      while (true) { 
        asm volatile ("serialize\n\t"); 
        if (time_diff.read(me, ORDER_READ) >= u || end_flag.read(me, ORDER_READ)) 
          break; 
      } // wait until the main thread goes to next chunk 
      if (end_flag.read(me, ORDER_READ))
        break; 
      u = time_diff.read(me, ORDER_READ); 
      local_counter = u; 
      start = u; 
    }
  }
}

void PrefetchThread_inner(int me, const Graph *g, NodeID start) { // for kron, twitter, and web (although not mem-intensive)
  /*-----pin the pf thread to specfic core-----*/ 
  pthread_t self = pthread_self();
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  // CPU_SET(me+64, &cpuset); // dublin 
  CPU_SET((me*2)+1, &cpuset); // beijing 
    
  int rc = pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpuset);
  if (rc != 0)
  {
    printf("Failed to pin pf thread.\n");
    exit(1);
  }
  /*-----pin the pf thread to specfic core-----*/
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
  start = time_diff.read(me, ORDER_READ); 
  for (NodeID u=start; u < g->num_nodes(); u++) { 
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
          size_t main_j = time_diff.read(me, ORDER_READ); 
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
  #pragma omp parallel
  {
    int me = omp_get_thread_num(); 
    /*-----pin the main thread to specfic core-----*/ 
    pthread_t self = pthread_self();
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    // CPU_SET(me, &cpuset);
    CPU_SET(me*2, &cpuset); // beijing
      
    int rc = pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpuset);
    if (rc != 0)
    {
      printf("Failed to pin main thread.\n");
      exit(1);
    }
    /*-----pin the main thread to specfic core-----*/

    /*-----compute boundary for each pf thread-----*/
    // size_t div = g.num_nodes() / NT; 
    // size_t mod = g.num_nodes() % NT; 
    // size_t head, tail; 
    // int previous_me = me - 1; 

    // if (me < mod) 
    //   tail = (div+1) * (me+1); 
    // else 
    //   tail = div*(me+1) + mod; 
    
    // if (previous_me < 0)
    //   head = 0; 
    // else if (previous_me < mod) 
    //   head = (div+1) * (previous_me+1); 
    // else 
    //   head = div*(previous_me+1) + mod; 
    /*-----compute boundary for each pf thread-----*/
    
    #if defined(HTPF) && !defined(INNER)
    size_t head = me*CHUNKSIZE; 
    time_diff.set(me, head, ORDER_WRITE); 
    end_flag.set(me, 0, ORDER_WRITE); 
    thread PF(PrefetchThread, me, &g, head); 
    #elif defined(HTPF) && defined(INNER)
    size_t head = me*CHUNKSIZE; 
    thread PF(PrefetchThread_inner, me, &g, head); 
    time_diff.set(me, 0, ORDER_WRITE); 
    #endif 
    #pragma omp for reduction(+ : total) schedule(dynamic, CHUNKSIZE)
    for (NodeID u=0; u < g.num_nodes(); u++) { 
      #if defined(HTPF) && !defined(INNER)
      time_diff.set(me, u, ORDER_WRITE); 
      #endif 
      for (NodeID v : g.out_neigh(u)) {
        if (v > u)
          break;
        auto it = g.out_neigh(v).begin(); 
        for (NodeID w : g.out_neigh(u)) {
          if (w > v)
            break;
          while (*it < w) 
            it++;
          if (w == *it)
            total++;
          #if defined(HTPF) && defined(INNER)
          time_diff.add(me, 1, ORDER_WRITE); // inner most sync 
          #endif 
        }
      }
    }
    #ifdef HTPF
    end_flag.set(me, 1, ORDER_WRITE); 
    PF.join(); 
    #endif 
  }
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
  omp_set_num_threads(NT); 

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
  BenchmarkKernel(cli, g, Hybrid, PrintTriangleStats, TCVerifier);

  return 0;
}
