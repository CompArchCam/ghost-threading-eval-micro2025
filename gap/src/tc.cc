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
#include <map> 

#include "omp.h"

#include "benchmark.h"
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "pvector.h"


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

#ifndef NT
#define NT 32
#endif

using namespace std;

// #define INNER_COUNT 
// #define INNER_MOST

#ifdef INNER_COUNT
#define UPPER_BOUNDARY 64
#define LOWER_BOUNDARY 0 
#define INTERESTING_BOUNDARY 32

static map<int, uint64_t> inner_histogram; 
uint64_t total_count, interesting_count; 
uint64_t max_count; 
#endif 

// #define SWPF

size_t OrderedCount(const Graph &g) {
  size_t total = 0;
  #pragma omp parallel for reduction(+ : total) schedule(dynamic, 64)
  for (NodeID u=0; u < g.num_nodes(); u++) { 
    #if defined(INNER_COUNT) && !defined(INNER_MOST)
    uint64_t count = 0; 
    #endif 
    for (NodeID* v = g.out_neigh(u).begin(); v < g.out_neigh(u).end(); v++) {
      #if defined(INNER_COUNT) && defined(INNER_MOST)
      uint64_t count = 0; 
      #endif 
      if (*v > u)
        break;
      #ifdef SWPF
      if (v + 64 < g.out_neigh(u).end()) {
        __builtin_prefetch(v + 64); 
        g.out_neigh(*(v + 32)).prefetch_begin(); 
      }
      #endif 
      auto it = g.out_neigh(*v).begin(); // 297.3, 26.8% 
      // firstly load *it 45.68, 48% 
      for (NodeID w : g.out_neigh(u)) {
        if (w > *v)
          break;
        while (*it < w) // re-load *it 11, 11%
          it++;
        if (w == *it)
          total++;
        #if defined(INNER_COUNT) && defined(INNER_MOST)
        count++; 
        total_count++; 
        #endif 
      }
      #if defined(INNER_COUNT) && defined(INNER_MOST)
      max_count = max_count >= count ? max_count : count; 
      if (count >= UPPER_BOUNDARY)
        inner_histogram[UPPER_BOUNDARY]++; 
      else if (count <= LOWER_BOUNDARY)
        inner_histogram[LOWER_BOUNDARY]++; 
      else 
        inner_histogram[count]++; 
      
      if (count > INTERESTING_BOUNDARY)
        interesting_count += count; 
      #endif 
      #if defined(INNER_COUNT) && !defined(INNER_MOST)
      count++; 
      total_count++; 
      #endif 
    }
    #if defined(INNER_COUNT) && !defined(INNER_MOST)
    max_count = max_count >= count ? max_count : count; 
    if (count >= UPPER_BOUNDARY)
      inner_histogram[UPPER_BOUNDARY]++; 
    else if (count <= LOWER_BOUNDARY)
      inner_histogram[LOWER_BOUNDARY]++; 
    else 
      inner_histogram[count]++; 
    
    if (count > INTERESTING_BOUNDARY)
      interesting_count += count; 
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
  #ifdef OMP
  omp_set_num_threads(NT); 
  #endif 

  #ifdef INNER_COUNT
  max_count = 0; 
  total_count = 0; 
  interesting_count = 0; 
  for (int i = LOWER_BOUNDARY; i <= UPPER_BOUNDARY; i++) {
    inner_histogram[i] = 0; 
  }
  #endif 

  CLApp cli(argc, argv, "triangle count");
  if (!cli.ParseArgs())
    return -1;
  Builder b(cli);
  Graph g = b.MakeGraph();
  if (g.directed()) {
    cout << "Input graph is directed but tc requires undirected" << endl;
    return -2;
  }
  BenchmarkKernel(cli, g, Hybrid, PrintTriangleStats, TCVerifier); // @profile: no start vertex? always do the same thing? 
  
  #ifdef INNER_COUNT
  for (int i = LOWER_BOUNDARY; i <= UPPER_BOUNDARY; i++) {
    cout << inner_histogram[i] << endl; 
  }
  cout << max_count << endl; 
  cout << interesting_count << " / " << total_count << ", " << (float)interesting_count/total_count << endl; 
  #endif 

  return 0;
}
