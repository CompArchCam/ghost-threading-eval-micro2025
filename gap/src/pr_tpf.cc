// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>

#include "omp.h"

#include "benchmark.h"
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "pvector.h"
#include "pf_support.h" 

/*
GAP Benchmark Suite
Kernel: PageRank (PR)
Author: Scott Beamer

Will return pagerank scores for all vertices once total change < epsilon

This PR implementation uses the traditional iterative approach. It performs
updates in the pull direction to remove the need for atomics, and it allows
new values to be immediately visible (like Gauss-Seidel method). The prior PR
implementation is still available in src/pr_spmv.cc.
*/

#define ORDER_READ memory_order_relaxed
#define ORDER_WRITE memory_order_relaxed

// #define HTPF
// #define INNER
// #define OMP
// #define TIME

using namespace std;

typedef float ScoreT;
const float kDamp = 0.85;

// static uint64_t sum_setup_time; 
// static uint64_t sum_wait_join_time; 

// #define WEB

#ifdef TIME

#define TOTAL_ITER 34745273064
#define FREQ 5000
uint32_t stamp_counter, array_counter; 
std::chrono::_V2::system_clock::time_point time_array[TOTAL_ITER/FREQ+100]; 

#if defined(HTPF)
  #if defined(KRON)
    #define OUTPUT "timestamp/pr.htpf.kron.csv"
  #elif defined(TWITTER)
    #define OUTPUT "timestamp/pr.htpf.twitter.csv"
  #elif defined(URAND)
    #define OUTPUT "timestamp/pr.htpf.urand.csv"
  #elif defined(ROAD)
    #define OUTPUT "timestamp/pr.htpf.road.csv"
  #elif defined(WEB)
    #define OUTPUT "timestamp/pr.htpf.web.csv"
  #endif 
#elif defined(OMP)
  #if defined(KRON)
    #define OUTPUT "timestamp/pr.homp.kron.csv"
  #elif defined(TWITTER)
    #define OUTPUT "timestamp/pr.homp.twitter.csv"
  #elif defined(URAND)
    #define OUTPUT "timestamp/pr.homp.urand.csv"
  #elif defined(ROAD)
    #define OUTPUT "timestamp/pr.homp.road.csv"
  #elif defined(WEB)
    #define OUTPUT "timestamp/pr.homp.web.csv"
  #endif 
#else
  #if defined(KRON)
    #define OUTPUT "timestamp/pr.kron.csv"
  #elif defined(TWITTER)
    #define OUTPUT "timestamp/pr.twitter.csv"
  #elif defined(URAND)
    #define OUTPUT "timestamp/pr.urand.csv"
  #elif defined(ROAD)
    #define OUTPUT "timestamp/pr.road.csv"
  #elif defined(WEB)
    #define OUTPUT "timestamp/pr.web.csv"
  #endif 
#endif 
#endif // TIME 

TimeDiff time_diff; 
HyperParam_PfT hyper_param; 

void PfThread(const Graph* g, ScoreT* const outgoing_contrib) {
    size_t local_counter = 0; 
    bool serialize_flag = false; 
    NodeID start_iter = time_diff.read_atomic_main(ORDER_READ); //0; 
    for (NodeID u=start_iter; u < g->num_nodes(); u++) {
      local_counter = u; 
      for (NodeID v : g->in_neigh(u)) {
        __builtin_prefetch(&outgoing_contrib[v]); // 0 for read, 2 for stay pos 
        // *(volatile ScoreT*)&outgoing_contrib[v]; // load 

        if (serialize_flag) {
          asm volatile ("serialize\n\t"); 
        }
      }
      sync<NodeID>(u, 1, u, false, time_diff, ORDER_READ, serialize_flag, hyper_param); 
    }
}

#ifdef INNER
void PfThread_inner(const Graph* g, ScoreT* const outgoing_contrib) {
    size_t local_counter = 0; 
    bool serialize_flag = false; 
    NodeID start_iter = time_diff.read_atomic_main(ORDER_READ); //0; 
    for (NodeID u=start_iter; u < g->num_nodes(); u++) {
      // uint32_t iter = g->in_neigh(u).end() - g->in_neigh(u).begin(); 
      for (NodeID* v = g->in_neigh(u).begin(); v < g->in_neigh(u).end(); v++) {
      // for (NodeID v : g->in_neigh(u)) {
        __builtin_prefetch(&outgoing_contrib[*v]); 
        // *(volatile ScoreT*)&outgoing_contrib[*v]; // load 
        if (serialize_flag) {
          asm volatile ("serialize\n\t"); 
        }
        /*----inner sync----*/
        local_counter++; 
        if (local_counter % hyper_param.sync_frequency == 0 || serialize_flag) { // when serialize enabled, check more often 
          // asm volatile ("serialize\n\t"); // a serialize here to make sure the counter is most up-to-date 
          size_t main_counter = time_diff.read_atomic_main(ORDER_READ); 
          if (main_counter >= local_counter) { // if pf thread is too slow 
            serialize_flag = false; 
            uint32_t remian_iter = g->in_neigh(u).end() - v; 
            if (main_counter - local_counter >= remian_iter) {
              break; 
            } else {
              v = v + (main_counter - local_counter) + hyper_param.skip_offset; 
            }
          } else if (local_counter - main_counter > hyper_param.serialize_threshold) { // pf thread is too fast 
            serialize_flag = true; 
          } else if (local_counter - main_counter < hyper_param.unserialize_threshold) {
            serialize_flag = false; 
          }
        }
        /*----inner sync----*/
      }
    }
}
#endif 

pvector<ScoreT> PageRankPullGS(const Graph &g, int max_iters, double epsilon=0,
                               bool logging_enabled = false) {
  const ScoreT init_score = 1.0f / g.num_nodes();
  const ScoreT base_score = (1.0f - kDamp) / g.num_nodes();
  pvector<ScoreT> scores(g.num_nodes(), init_score);
  pvector<ScoreT> outgoing_contrib(g.num_nodes());
  #pragma omp parallel for
  for (NodeID n=0; n < g.num_nodes(); n++)
    outgoing_contrib[n] = init_score / g.out_degree(n);
  for (int iter=0; iter < max_iters; iter++) {
    double error = 0;

    const auto gp = &g; 
    const auto op = outgoing_contrib.begin(); 
    // const auto start = chrono::high_resolution_clock::now();
    #ifdef HTPF
    #ifdef INNER
    thread PF(PfThread_inner, gp, op); 
    time_diff.set_atomic_main(0, ORDER_WRITE); // inner sync
    #else
    thread PF(PfThread, gp, op); // outer 
    #endif // INNER
    #endif // HTPF
    // const auto end = chrono::high_resolution_clock::now();
    // auto us_int = chrono::duration_cast<chrono::microseconds>(end - start);
    // cout << "thread setup time: " << us_int.count() << "us" << endl; 
    // sum_setup_time += us_int.count(); 
    #pragma omp parallel for reduction(+ : error) schedule(dynamic, 16384)
    for (NodeID u=0; u < g.num_nodes(); u++) {
      #if defined(HTPF) && !defined(INNER)
      time_diff.set_atomic_main(u, ORDER_WRITE); // not inner sync 
      #endif 
      ScoreT incoming_total = 0;
      for (NodeID v : g.in_neigh(u)) {
        incoming_total += outgoing_contrib[v];
        #if defined(HTPF) && defined(INNER)
        time_diff.add_atomic_main(1, ORDER_WRITE); // inner sync
        #endif 
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
      ScoreT old_score = scores[u];
      scores[u] = base_score + kDamp * incoming_total;
      error += fabs(scores[u] - old_score);
      outgoing_contrib[u] = scores[u] / g.out_degree(u);
    }
    #ifdef HTPF
    PF.join(); 
    #endif 

    if (logging_enabled)
      PrintStep(iter, error);
    if (error < epsilon)
      break;
  }
  return scores;
}


void PrintTopScores(const Graph &g, const pvector<ScoreT> &scores) {
  vector<pair<NodeID, ScoreT>> score_pairs(g.num_nodes());
  for (NodeID n=0; n < g.num_nodes(); n++) {
    score_pairs[n] = make_pair(n, scores[n]);
  }
  int k = 5;
  vector<pair<ScoreT, NodeID>> top_k = TopK(score_pairs, k);
  for (auto kvp : top_k)
    cout << kvp.second << ":" << kvp.first << endl;
}


// Verifies by asserting a single serial iteration in push direction has
//   error < target_error
bool PRVerifier(const Graph &g, const pvector<ScoreT> &scores,
                        double target_error) {
  const ScoreT base_score = (1.0f - kDamp) / g.num_nodes();
  pvector<ScoreT> incoming_sums(g.num_nodes(), 0);
  double error = 0;
  for (NodeID u : g.vertices()) {
    ScoreT outgoing_contrib = scores[u] / g.out_degree(u);
    for (NodeID v : g.out_neigh(u))
      incoming_sums[v] += outgoing_contrib;
  }
  for (NodeID n : g.vertices()) {
    error += fabs(base_score + kDamp * incoming_sums[n] - scores[n]);
    incoming_sums[n] = 0;
  }
  PrintTime("Total Error", error);
  return error < target_error;
}


int main(int argc, char* argv[]) {
  #ifdef OMP
  omp_set_num_threads(2); 
  #endif

  time_diff.init_atomic(); 
  // time_diff.init_atomic_histogram(751619276); 
  #ifdef TIME
  stamp_counter = 0; 
  array_counter = 0; 
  #endif 

  // sum_setup_time = 0; 
  // sum_wait_join_time = 0; 
  CLPageRank cli(argc, argv, "pagerank", 1e-4, 20);
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
  auto PRBound = [&cli] (const Graph &g) {
    return PageRankPullGS(g, cli.max_iters(), cli.tolerance(), cli.logging_en());
  };
  auto VerifierBound = [&cli] (const Graph &g, const pvector<ScoreT> &scores) {
    return PRVerifier(g, scores, cli.tolerance());
  };
  #ifdef TIME
  auto kernel_start = chrono::high_resolution_clock::now();
  #endif 
  BenchmarkKernel(cli, g, PRBound, PrintTopScores, VerifierBound);

  // cout << "total thread setup time = " << sum_setup_time << "us" << endl; 
  // cout << "total thread wait join time = " << sum_wait_join_time << "us" << endl; 

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
