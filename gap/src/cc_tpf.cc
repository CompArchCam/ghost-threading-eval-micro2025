// Copyright (c) 2018, The Hebrew University of Jerusalem (HUJI, A. Barak)
// See LICENSE.txt for license details

#include <algorithm>
#include <cinttypes>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <thread>
#include <cassert>

#include "omp.h"

#include "benchmark.h"
#include "bitmap.h"
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "pvector.h"
#include "pf_support.h"


/*
GAP Benchmark Suite
Kernel: Connected Components (CC)
Authors: Michael Sutton, Scott Beamer

Will return comp array labelling each vertex with a connected component ID

This CC implementation makes use of the Afforest subgraph sampling algorithm [1],
which restructures and extends the Shiloach-Vishkin algorithm [2].

[1] Michael Sutton, Tal Ben-Nun, and Amnon Barak. "Optimizing Parallel 
    Graph Connectivity Computation via Subgraph Sampling" Symposium on 
    Parallel and Distributed Processing, IPDPS 2018.

[2] Yossi Shiloach and Uzi Vishkin. "An o(logn) parallel connectivity algorithm"
    Journal of Algorithms, 3(1):57–67, 1982.
*/

#define ORDER_READ memory_order_relaxed
#define ORDER_WRITE memory_order_relaxed

// #define HTPF
// #define OMP
// #define TIME
// #define TIMESTAMP

// #define URAND
// #define TUNING

#ifdef TIME
#define TOTAL_ITER 856706060
#define FREQ 1000
uint32_t stamp_counter, array_counter; 
std::chrono::_V2::system_clock::time_point time_array[TOTAL_ITER/FREQ+100]; 

#if defined(HTPF)
  #if defined(KRON)
    #define OUTPUT "timestamp/cc.htpf.kron.csv"
  #elif defined(TWITTER)
    #define OUTPUT "timestamp/cc.htpf.twitter.csv"
  #elif defined(URAND)
    #define OUTPUT "timestamp/cc.htpf.urand.csv"
  #elif defined(ROAD)
    #define OUTPUT "timestamp/cc.htpf.road.csv"
  #elif defined(WEB)
    #define OUTPUT "timestamp/cc.htpf.web.csv"
  #endif 
#elif defined(OMP)
  #if defined(KRON)
    #define OUTPUT "timestamp/cc.homp.kron.csv"
  #elif defined(TWITTER)
    #define OUTPUT "timestamp/cc.homp.twitter.csv"
  #elif defined(URAND)
    #define OUTPUT "timestamp/cc.homp.urand.csv"
  #elif defined(ROAD)
    #define OUTPUT "timestamp/cc.homp.road.csv"
  #elif defined(WEB)
    #define OUTPUT "timestamp/cc.homp.web.csv"
  #endif 
#else
  #if defined(KRON)
    #define OUTPUT "timestamp/cc.kron.csv"
  #elif defined(TWITTER)
    #define OUTPUT "timestamp/cc.twitter.csv"
  #elif defined(URAND)
    #define OUTPUT "timestamp/cc.urand.csv"
  #elif defined(ROAD)
    #define OUTPUT "timestamp/cc.road.csv"
  #elif defined(WEB)
    #define OUTPUT "timestamp/cc.web.csv"
  #endif 
#endif 
#endif 

#ifdef TIMESTAMP
int* timestamp_array; 
uint32_t* iter_number;
uint32_t array_counter; 

#if defined(KRON)
    #define OUTPUT "timestamp/cc.htpf.kron.csv"
  #elif defined(TWITTER)
    #define OUTPUT "timestamp/cc.htpf.twitter.csv"
  #elif defined(URAND)
    #define OUTPUT "timestamp/cc.htpf.urand.csv"
  #elif defined(ROAD)
    #define OUTPUT "timestamp/cc.htpf.road.csv"
  #elif defined(WEB)
    #define OUTPUT "timestamp/cc.htpf.web.csv"
  #endif 
#endif 

using namespace std;

TimeDiff time_diff; 
HyperParam_PfT hyper_param; 

void PrefetchThread_web(const Graph *g, int r, NodeID *comp, NodeID *end) {
  #if defined(WEB) && !defined(TUNING)
  HyperParam_PfT hyperparam = {.sync_frequency = 800, .skip_offset = 130, 
                               .serialize_threshold = 150, .unserialize_threshold = 100}; 
  #else
  HyperParam_PfT hyperparam = hyper_param; 
  #endif 
  bool serialize_flag = false; 
  for (NodeID u = 0; u < g->num_nodes(); u++) { 
    for (const NodeID &v : g->out_neigh(u, r)) { // 134217728 iter per invoc 
      /*-------Link()-------*/
      __builtin_prefetch(&v); 
      if (serialize_flag)
        asm volatile ("serialize\n\t"); 
      /*-------Link()-------*/
      break;
    }
    if (serialize_flag)
      asm volatile ("serialize\n\t"); 
    #ifdef TIMESTAMP
    if (u % 100 == 0) {
      int diff = u - time_diff.read_atomic_main(ORDER_READ); 
      timestamp_array[array_counter] = diff; 
      array_counter++; 
    }
    #endif 
    sync<NodeID>(u, 1, u, true, time_diff, ORDER_READ, serialize_flag, hyperparam); 
  }
}

void PrefetchThread_kron_twitter(const Graph *g, int r, NodeID *comp, NodeID *end) {
  #if (defined(KRON) || defined(TWITTER)) && !defined(TUNING)
  HyperParam_PfT hyperparam = {.sync_frequency = 800, .skip_offset = 450, 
                               .serialize_threshold = 800, .unserialize_threshold = 750}; 
  #else
  HyperParam_PfT hyperparam = hyper_param; 
  #endif 
  bool serialize_flag = false; 
  for (NodeID u = 0; u < g->num_nodes(); u++) { 
    for (NodeID v : g->out_neigh(u, r)) { // 134217728 iter per invoc 
      /*-------Link()-------*/
      __builtin_prefetch(&comp[v]); 
      if (serialize_flag)
        asm volatile ("serialize\n\t"); 
      /*-------Link()-------*/
      break;
    }
    if (serialize_flag)
      asm volatile ("serialize\n\t"); 
    #ifdef TIMESTAMP
    if (u % 100 == 0) {
      int diff = u - time_diff.read_atomic_main(ORDER_READ); 
      timestamp_array[array_counter] = diff; 
      array_counter++; 
    }
    #endif 
    sync<NodeID>(u, 1, u, true, time_diff, ORDER_READ, serialize_flag, hyperparam); 
  }
}

void PrefetchThread_urand(const Graph *g, int r, NodeID *comp, NodeID *end) {
  #if defined(URAND) && !defined(TUNING)
  HyperParam_PfT hyperparam = {.sync_frequency = 20, .skip_offset = 20, 
                               .serialize_threshold = 45, .unserialize_threshold = 42}; 
  #else
  HyperParam_PfT hyperparam = hyper_param; 
  #endif 
  #ifdef TIMESTAMP
  uint32_t local_counter = 0; 
  #endif 
  bool serialize_flag = false; 
  for (NodeID u = 0; u < g->num_nodes(); u++) { 
    // NodeID tmp; 
    for (NodeID v : g->out_neigh(u, r)) { // 134217728 iter per invoc 
      /*-------Link()-------*/
      NodeID p1 = comp[u];
      NodeID p2 = comp[v]; // 142 cpi, 29.8 coverage 
      while (p1 != p2) { // iter per invoc 3.3 
        NodeID high = p1 > p2 ? p1 : p2;
        NodeID low = p1 + (p2 - high);
        NodeID p_high = comp[high]; // 50.5, 33.8% 
        if ((p_high == low) || (p_high == high))
          break;
        p1 = comp[comp[high]]; // 39.3 cpi, 18.9% coverage, but prefetch both p1 and p2 
        p2 = comp[low];
        if (serialize_flag)
          asm volatile ("serialize\n\t"); 
      }
      /*-------Link()-------*/
      #ifdef TIMESTAMP
      local_counter++; 
      #endif 
      break;
    }
    sync<NodeID>(u, 1, u, true, time_diff, ORDER_READ, serialize_flag, hyperparam); 
    #ifdef TIMESTAMP
    if (local_counter % 10 == 0) {
      int diff = u - ((int) time_diff.read_atomic_main(ORDER_READ)); 
      timestamp_array[array_counter] = diff; 
      iter_number[array_counter] = u;
      array_counter++; 
    }
    #endif 
  }
}

// Place nodes u and v in same component of lower component ID
void Link(NodeID u, NodeID v, pvector<NodeID>& comp) {
  NodeID p1 = comp[u];
  NodeID p2 = comp[v]; // 142 cpi, 29.8 coverage 
  while (p1 != p2) { // iter per invoc 3.3 
    NodeID high = p1 > p2 ? p1 : p2;
    NodeID low = p1 + (p2 - high);
    NodeID p_high = comp[high]; // 50.5, 33.8% 
    // Was already 'low' or succeeded in writing 'low'
    if ((p_high == low) ||
        (p_high == high && compare_and_swap(comp[high], high, low)))
      break;
    p1 = comp[comp[high]]; // 39.3 cpi, 18.9% coverage, but prefetch both p1 and p2 
    p2 = comp[low];
  }
}


// Reduce depth of tree for each component to 1 by crawling up parents
void Compress(const Graph &g, pvector<NodeID>& comp) {
  #pragma omp parallel for schedule(dynamic, 16384)
  for (NodeID n = 0; n < g.num_nodes(); n++) {
    while (comp[n] != comp[comp[n]]) {
      comp[n] = comp[comp[n]];
    }
  }
}


NodeID SampleFrequentElement(const pvector<NodeID>& comp,
                             bool logging_enabled = false,
                             int64_t num_samples = 1024) {
  std::unordered_map<NodeID, int> sample_counts(32);
  using kvp_type = std::unordered_map<NodeID, int>::value_type;
  // Sample elements from 'comp'
  std::mt19937 gen;
  std::uniform_int_distribution<NodeID> distribution(0, comp.size() - 1);
  for (NodeID i = 0; i < num_samples; i++) {
    NodeID n = distribution(gen);
    sample_counts[comp[n]]++;
  }
  // Find most frequent element in samples (estimate of most frequent overall)
  auto most_frequent = std::max_element(
    sample_counts.begin(), sample_counts.end(),
    [](const kvp_type& a, const kvp_type& b) { return a.second < b.second; });
  float frac_of_graph = static_cast<float>(most_frequent->second) / num_samples;
  if (logging_enabled)
    std::cout
      << "Skipping largest intermediate component (ID: " << most_frequent->first
      << ", approx. " << static_cast<int>(frac_of_graph * 100)
      << "% of the graph)" << std::endl;
  return most_frequent->first;
}


pvector<NodeID> Afforest(const Graph &g, bool logging_enabled = false,
                         int32_t neighbor_rounds = 2) {
  pvector<NodeID> comp(g.num_nodes());

  // Initialize each node to a single-node self-pointing tree
  #pragma omp parallel for
  for (NodeID n = 0; n < g.num_nodes(); n++)
    comp[n] = n;

  // Process a sparse sampled subgraph first for approximating components.
  // Sample by processing a fixed number of neighbors for each node (see paper)
  for (int r = 0; r < neighbor_rounds; ++r) {
    #ifdef HTPF
    #if defined(URAND)
    thread PF(PrefetchThread_urand, &g, r, comp.begin(), comp.end()); 
    #elif defined(KRON) || defined(TWITTER) || defined(ROAD)
    thread PF(PrefetchThread_kron_twitter, &g, r, comp.begin(), comp.end()); 
    #elif defined(WEB)
    thread PF(PrefetchThread_web, &g, r, comp.begin(), comp.end()); 
    #endif 
    #endif 
  #pragma omp parallel for schedule(dynamic,16384)
    for (NodeID u = 0; u < g.num_nodes(); u++) { 
      #ifdef HTPF
      time_diff.set_atomic_main((size_t) u, ORDER_WRITE); 
      #endif 
      for (NodeID v : g.out_neigh(u, r)) { // 134217728 iter per invoc 
        // Link at most one time if neighbor available at offset r
        Link(u, v, comp);
        break;
      }
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
    }
    Compress(g, comp);
    #ifdef HTPF
    PF.join(); 
    #endif 
  }

  // Sample 'comp' to find the most frequent element -- due to prior
  // compression, this value represents the largest intermediate component
  NodeID c = SampleFrequentElement(comp, logging_enabled);

  // Final 'link' phase over remaining edges (excluding the largest component)
  if (!g.directed()) {
    #pragma omp parallel for schedule(dynamic, 16384)
    for (NodeID u = 0; u < g.num_nodes(); u++) {
      // Skip processing nodes in the largest component
      if (comp[u] == c)
        continue;
      // Skip over part of neighborhood (determined by neighbor_rounds)
      for (NodeID v : g.out_neigh(u, neighbor_rounds)) {
        Link(u, v, comp);
      }
    }
  } else {
    #pragma omp parallel for schedule(dynamic, 16384)
    for (NodeID u = 0; u < g.num_nodes(); u++) {
      if (comp[u] == c)
        continue;
      for (NodeID v : g.out_neigh(u, neighbor_rounds)) {
        Link(u, v, comp);
      }
      // To support directed graphs, process reverse graph completely
      for (NodeID v : g.in_neigh(u)) {
        Link(u, v, comp);
      }
    }
  }
  // Finally, 'compress' for final convergence
  Compress(g, comp);
  return comp;
}


void PrintCompStats(const Graph &g, const pvector<NodeID> &comp) {
  cout << endl;
  unordered_map<NodeID, NodeID> count;
  for (NodeID comp_i : comp)
    count[comp_i] += 1;
  int k = 5;
  vector<pair<NodeID, NodeID>> count_vector;
  count_vector.reserve(count.size());
  for (auto kvp : count)
    count_vector.push_back(kvp);
  vector<pair<NodeID, NodeID>> top_k = TopK(count_vector, k);
  k = min(k, static_cast<int>(top_k.size()));
  cout << k << " biggest clusters" << endl;
  for (auto kvp : top_k)
    cout << kvp.second << ":" << kvp.first << endl;
  cout << "There are " << count.size() << " components" << endl;
}


// Verifies CC result by performing a BFS from a vertex in each component
// - Asserts search does not reach a vertex with a different component label
// - If the graph is directed, it performs the search as if it was undirected
// - Asserts every vertex is visited (degree-0 vertex should have own label)
bool CCVerifier(const Graph &g, const pvector<NodeID> &comp) {
  unordered_map<NodeID, NodeID> label_to_source;
  for (NodeID n : g.vertices())
    label_to_source[comp[n]] = n;
  Bitmap visited(g.num_nodes());
  visited.reset();
  vector<NodeID> frontier;
  frontier.reserve(g.num_nodes());
  for (auto label_source_pair : label_to_source) {
    NodeID curr_label = label_source_pair.first;
    NodeID source = label_source_pair.second;
    frontier.clear();
    frontier.push_back(source);
    visited.set_bit(source);
    for (auto it = frontier.begin(); it != frontier.end(); it++) {
      NodeID u = *it;
      for (NodeID v : g.out_neigh(u)) {
        if (comp[v] != curr_label)
          return false;
        if (!visited.get_bit(v)) {
          visited.set_bit(v);
          frontier.push_back(v);
        }
      }
      if (g.directed()) {
        for (NodeID v : g.in_neigh(u)) {
          if (comp[v] != curr_label)
            return false;
          if (!visited.get_bit(v)) {
            visited.set_bit(v);
            frontier.push_back(v);
          }
        }
      }
    }
  }
  for (NodeID n=0; n < g.num_nodes(); n++)
    if (!visited.get_bit(n))
      return false;
  return true;
}


int main(int argc, char* argv[]) {
  #ifdef OMP
  omp_set_num_threads(2); 
  #endif

  #ifdef TIME
  stamp_counter = 0; 
  array_counter = 0; 
  #endif 
  #ifdef TIMESTAMP
  timestamp_array = new int[268435456]; 
  iter_number = new uint32_t[268435456];
  array_counter = 0; 
  #endif 
  time_diff.init_atomic(); 
  // time_diff.init_atomic_histogram(268435452); 
  CLApp cli(argc, argv, "connected-components-afforest");
  if (!cli.ParseArgs())
    return -1;
  hyper_param.sync_frequency = cli.sync_frequency(); 
  hyper_param.skip_offset = cli.skip_offset(); 
  hyper_param.serialize_threshold = cli.serialize_threshold(); 
  hyper_param.unserialize_threshold = cli.serialize_threshold() > cli.unserialize_threshold() ? 
            cli.serialize_threshold() - cli.unserialize_threshold() : 0; 
  cout << "sync frequency = " << hyper_param.sync_frequency << ", serialize threshold = " << hyper_param.serialize_threshold 
       << ", unserialize threshold = " << hyper_param.unserialize_threshold << ", skip offset = " << hyper_param.skip_offset << endl; 
  Builder b(cli);
  Graph g = b.MakeGraph();
  auto CCBound = [&cli](const Graph& gr){ return Afforest(gr, cli.logging_en()); };
  #ifdef TIME
  auto kernel_start = chrono::high_resolution_clock::now();
  #endif 
  BenchmarkKernel(cli, g, CCBound, PrintCompStats, CCVerifier);
  // time_diff.print_atomic_histogram(); 
  #ifdef TIME
  ofstream myout; 
  myout.open(OUTPUT); 
  for (uint32_t i = 0; i < array_counter; i++) {
    // time_diff[i] = chrono::duration_cast<chrono::nanoseconds>(time_array[i] - kernel_start).count(); 
    myout << chrono::duration_cast<chrono::microseconds>(time_array[i] - kernel_start).count() << endl; 
  }
  cout << stamp_counter; 
  #endif 
  #ifdef TIMESTAMP
  ofstream myout; 
  myout.open(OUTPUT); 
  for (uint32_t i = 0; i < array_counter; i++) {
    myout << iter_number[i] << ',' << timestamp_array[i] << endl; 
  }
  free(timestamp_array);
  free(iter_number);
  #endif 
  return 0;
}
