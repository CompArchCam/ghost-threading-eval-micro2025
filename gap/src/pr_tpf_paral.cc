// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#include <algorithm>
#include <iostream>
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

#ifndef NT
#define NT 2
#endif 


#define ORDER_READ memory_order_relaxed
#define ORDER_WRITE memory_order_relaxed

using namespace std;

typedef float ScoreT;
const float kDamp = 0.85;

OMPSyncAtomic time_diff(NT); 
HyperParam_PfT hyper_param; 

// TimeDiff histogram; 

void PfThread(int me, const Graph* g, int64_t start, int64_t end, ScoreT* const outgoing_contrib) {
  /*-----pin the pf thread to specfic core-----*/ 
  pthread_t self = pthread_self();
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(me*2+1, &cpuset);
    
  int rc = pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpuset);
  if (rc != 0)
  {
    printf("Failed to pin main thread.\n");
    exit(1);
  }
  /*-----pin the pf thread to specfic core-----*/ 
  bool serialize_flag = false; 
  for (NodeID u=start; u < end; u++) {
    for (NodeID v : g->in_neigh(u)) {
      __builtin_prefetch(&outgoing_contrib[v]); 
      // *(volatile ScoreT*)&outgoing_contrib[v]; // load 
      if (serialize_flag) 
        asm volatile ("serialize\n\t"); 
    }
    sync<NodeID>(u, 1, u, true, time_diff, me, ORDER_READ, serialize_flag, hyper_param); 
    // if (u % 4 == 0 && me == 0)
    //   histogram.insert_into_atomic_histogram(time_diff.read(me, ORDER_READ), u); 
  }
}

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
    // compute start/end point of each thread 
    #pragma omp parallel 
    {
    int me = omp_get_thread_num(); 
    /*-----pin the pf thread to specfic core-----*/ 
    pthread_t self = pthread_self();
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(me*2, &cpuset);
          
    int rc = pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
      printf("Failed to pin main thread.\n");
      exit(1);
    }
    /*-----pin the pf thread to specfic core-----*/ 
    // vector<thread> pf_threads; 
    size_t div = g.num_nodes() / NT; 
    size_t mod = g.num_nodes() % NT; 
    size_t head, tail; 
    // bool affinity_flag[NT] = {false}; 
    // for (int me = 0; me < NT; me++) {
      /* compute boundary for each pf thread */
      // int previous_me = me - 1; 
      if (me - 1 < 0)
        head = 0; 
      else if (me - 1 < mod) 
        head = (div+1) * me; 
      else 
        head = div*me + mod; 
      
      if (me < mod) 
        tail = (div+1) * (me+1); 
      else 
        tail = div*(me+1) + mod; 
      /* compute boundary for each pf thread */
      // time_diff.set(me, head, ORDER_WRITE); 
      thread PF(PfThread, me, &g, head, tail, outgoing_contrib.begin()); 
      // pf_threads.push_back(thread(PfThread, me, &g, tail, outgoing_contrib.begin())); // issue thread 
    // }
    // cout << me << ' ' << head << ' ' << tail << ' ' << g.num_nodes() << endl; 
    #pragma omp for reduction(+ : error) schedule(static) //schedule(dynamic, 16384)
    for (NodeID u=0; u < g.num_nodes(); u++) {
      ScoreT incoming_total = 0;
      for (NodeID v : g.in_neigh(u)) {
        incoming_total += outgoing_contrib[v];
      }
      ScoreT old_score = scores[u];
      scores[u] = base_score + kDamp * incoming_total;
      error += fabs(scores[u] - old_score);
      outgoing_contrib[u] = scores[u] / g.out_degree(u);
      time_diff.set(me, u+1, ORDER_WRITE); 
    }
    // for (auto &pf_thread: pf_threads)
    //   pf_thread.join(); 
    PF.join(); 
    }

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
  omp_set_num_threads(NT); 
  // histogram.init_atomic_histogram(17179866960/8); 

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
  BenchmarkKernel(cli, g, PRBound, PrintTopScores, VerifierBound);

  // histogram.print_atomic_histogram(); 
  return 0;
}
