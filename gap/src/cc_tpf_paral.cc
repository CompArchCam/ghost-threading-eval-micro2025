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

#ifndef NT
#define NT 2
#endif 

#ifndef CHUNKSIZE
#define CHUNKSIZE 16384
#endif 

// #define HTPF 

#define ORDER_READ memory_order_relaxed
#define ORDER_WRITE memory_order_relaxed

using namespace std;

OMPSyncAtomic time_diff(NT); 
OMPSyncAtomic end_flag(NT); 
HyperParam_PfT hyper_param; 

void PrefetchThread_web(int me, const Graph *g, int r, NodeID *comp, size_t start, size_t end) {
  /*-----pin the pf thread to specfic core-----*/ 
  pthread_t self = pthread_self();
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  // CPU_SET(me+64, &cpuset); // dublin 
  CPU_SET((me*2)+1, &cpuset); // beijing 
    
  int rc = pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpuset);
  if (rc != 0)
  {
    printf("Failed to pin main thread.\n");
    exit(1);
  }
  /*-----pin the pf thread to specfic core-----*/
  HyperParam_PfT hyperparam = hyper_param; 
  bool serialize_flag = false; 
  for (NodeID u = start; u < end; u++) { 
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
    sync<NodeID>(u, 1, u, true, time_diff, me, ORDER_READ, serialize_flag, hyperparam); 
  }
}

void PrefetchThread_kron_twitter(int me, const Graph *g, int r, NodeID *comp, size_t start, size_t end) {
  /*-----pin the pf thread to specfic core-----*/ 
  pthread_t self = pthread_self();
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  // CPU_SET(me+64, &cpuset); // dublin 
  CPU_SET((me*2)+1, &cpuset); // beijing 
    
  int rc = pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpuset);
  if (rc != 0)
  {
    printf("Failed to pin main thread.\n");
    exit(1);
  }
  /*-----pin the pf thread to specfic core-----*/

  HyperParam_PfT hyperparam = hyper_param; 
  bool serialize_flag = false; 
  start = time_diff.read(me, ORDER_READ) + hyperparam.skip_offset; 
  NodeID local_counter = start; 
  for (NodeID u = start; u < g->num_nodes(); u++) { 
    local_counter = u - start; 
    for (NodeID v : g->out_neigh(u, r)) { 
      /*-------Link()-------*/
      __builtin_prefetch(&comp[v]); 
      if (serialize_flag)
        asm volatile ("serialize\n\t"); 
      /*-------Link()-------*/
      break;
    }
    if (serialize_flag)
      asm volatile ("serialize\n\t"); 
    sync<NodeID>(u, 1, u, true, time_diff, me, ORDER_READ, serialize_flag, hyperparam); 
    if (local_counter >= CHUNKSIZE) {
      while (true) { 
        if (time_diff.read(me, ORDER_READ) >= u || end_flag.read(me, ORDER_READ)) 
          break; 
        asm volatile ("serialize\n\t"); 
      } // wait until the main thread goes to next chunk 
      if (end_flag.read(me, ORDER_READ))
        break; 
      u = time_diff.read(me, ORDER_READ); 
      local_counter = u; 
      start = u; 
    }
  }
}

void PrefetchThread_urand(int me, const Graph *g, int r, NodeID *comp, size_t start, size_t end) {
  /*-----pin the pf thread to specfic core-----*/ 
  pthread_t self = pthread_self();
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  // CPU_SET(me+64, &cpuset); // dublin 
  CPU_SET((me*2)+1, &cpuset); // beijing 
    
  int rc = pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpuset);
  if (rc != 0)
  {
    printf("Failed to pin main thread.\n");
    exit(1);
  }
  /*-----pin the pf thread to specfic core-----*/
  HyperParam_PfT hyperparam = hyper_param; 
  bool serialize_flag = false; 
  start = time_diff.read(me, ORDER_READ) + hyperparam.skip_offset; 
  for (NodeID u = start; u < end; u++) { 
    for (NodeID v : g->out_neigh(u, r)) { 
      /*-------Link()-------*/
      NodeID p1 = comp[u];
      NodeID p2 = comp[v]; 
      while (p1 != p2) { 
        NodeID high = p1 > p2 ? p1 : p2;
        NodeID low = p1 + (p2 - high);
        NodeID p_high = comp[high]; 
        if ((p_high == low) || (p_high == high))
          break;
        p1 = comp[comp[high]]; 
        p2 = comp[low];
        if (serialize_flag)
          asm volatile ("serialize\n\t"); 
      }
      /*-------Link()-------*/
      break;
    }
    sync<NodeID>(u, 1, u, true, time_diff, me, ORDER_READ, serialize_flag, hyperparam); 
  }
}

// Place nodes u and v in same component of lower component ID
void Link(NodeID u, NodeID v, pvector<NodeID>& comp) {
  NodeID p1 = comp[u];
  NodeID p2 = comp[v]; 
  while (p1 != p2) { 
    NodeID high = p1 > p2 ? p1 : p2;
    NodeID low = p1 + (p2 - high);
    NodeID p_high = comp[high]; 
    // Was already 'low' or succeeded in writing 'low'
    if ((p_high == low) ||
        (p_high == high && compare_and_swap(comp[high], high, low)))
      break;
    p1 = comp[comp[high]]; 
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
    #pragma omp parallel
    {
        int me = omp_get_thread_num(); 
        /*-----pin the pf thread to specfic core-----*/ 
        pthread_t self = pthread_self();
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        // CPU_SET(me, &cpuset);
        CPU_SET(me*2, &cpuset); // beijing
                    
        int rc = pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            printf("Failed to pin main thread.\n");
            exit(1);
        }
        /*-----pin the pf thread to specfic core-----*/
    
        /*-----compute boundary for each pf thread-----*/
        #if defined(URAND) || defined(WEB)
        size_t div = g.num_nodes() / NT; 
        size_t mod = g.num_nodes() % NT; 
        size_t head, tail; 
        int previous_me = me - 1; 
    
        if (me < mod) 
            tail = (div+1) * (me+1); 
        else 
            tail = div*(me+1) + mod; 
            
        if (previous_me < 0)
            head = 0; 
        else if (previous_me < mod) 
            head = (div+1) * (previous_me+1); 
        else 
            head = div*(previous_me+1) + mod; 
        #endif 
        /*-----compute boundary for each pf thread-----*/
        #ifdef HTPF
        #if defined(URAND)
        thread PF(PrefetchThread_urand, me, &g, r, comp.begin(), head, tail); 
        #elif defined(KRON) || defined(TWITTER) || defined(ROAD)
        time_diff.set(me, 0, ORDER_WRITE); 
        end_flag.set(me, 0, ORDER_WRITE); 
        size_t head = 0 + me*CHUNKSIZE; 
        thread PF(PrefetchThread_kron_twitter, me, &g, r, comp.begin(), head, 0); 
        #elif defined(WEB)
        thread PF(PrefetchThread_web, me, &g, r, comp.begin(), head, tail); 
        #endif 
        #endif 
        // #pragma omp parallel for schedule(dynamic,16384)
        #if defined(URAND) || defined(WEB)
        #pragma omp for schedule(static)
        #else
        #pragma omp for schedule(dynamic,CHUNKSIZE)
        #endif 
        for (NodeID u = 0; u < g.num_nodes(); u++) { 
          #ifdef HTPF
          time_diff.set(me, (size_t) u, ORDER_WRITE); 
          #endif 
          for (NodeID v : g.out_neigh(u, r)) { 
              // Link at most one time if neighbor available at offset r
              Link(u, v, comp);
              break;
          }
        }
        #ifdef HTPF
        #if defined(KRON) || defined(TWITTER) || defined(ROAD)
        end_flag.set(me, 1, ORDER_WRITE); 
        #endif 
        PF.join(); 
        #endif 
    } // omp parallel 
        Compress(g, comp);
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
  omp_set_num_threads(NT); 

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
  BenchmarkKernel(cli, g, CCBound, PrintCompStats, CCVerifier);
  // time_diff.print_atomic_histogram(); 

  return 0;
}
