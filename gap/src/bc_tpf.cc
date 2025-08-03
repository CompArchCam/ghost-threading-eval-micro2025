// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#include <functional>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread> 

#include "omp.h"

#include "benchmark.h"
#include "bitmap.h"
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "platform_atomics.h"
#include "pvector.h"
#include "sliding_queue.h"
#include "timer.h"
#include "util.h"

#include "pf_support.h"


/*
GAP Benchmark Suite
Kernel: Betweenness Centrality (BC)
Author: Scott Beamer

Will return array of approx betweenness centrality scores for each vertex

This BC implementation makes use of the Brandes [1] algorithm with
implementation optimizations from Madduri et al. [2]. It is only approximate
because it does not compute the paths from every start vertex, but only a small
subset of them. Additionally, the scores are normalized to the range [0,1].

As an optimization to save memory, this implementation uses a Bitmap to hold
succ (list of successors) found during the BFS phase that are used in the back-
propagation phase.

[1] Ulrik Brandes. "A faster algorithm for betweenness centrality." Journal of
    Mathematical Sociology, 25(2):163–177, 2001.

[2] Kamesh Madduri, David Ediger, Karl Jiang, David A Bader, and Daniel
    Chavarria-Miranda. "A faster parallel algorithm and efficient multithreaded
    implementations for evaluating betweenness centrality on massive datasets."
    International Symposium on Parallel & Distributed Processing (IPDPS), 2009.
*/

#define ORDER_READ memory_order_relaxed
#define ORDER_WRITE memory_order_relaxed

// #define HTPF 
// #define INNER 
// #define OMP
// #define TUNING

// #define TIME 

// #define KRON

#if defined(HTPF) && !defined(URAND)
#define INNER
#endif 

#if defined(KRON) || defined(TWITTER) || defined(ROAD)
#define FIRST
#endif 

#ifdef WEB
#define BEST
#endif 

#ifdef TIME
#define TOTAL_ITER 4294966740
#define FREQ 1000
uint32_t stamp_counter, array_counter; 
std::chrono::_V2::system_clock::time_point time_array[TOTAL_ITER/FREQ+100]; 

#if defined(HTPF)
  #if defined(KRON)
    #define OUTPUT "timestamp/bc.htpf.kron.csv"
  #elif defined(TWITTER)
    #define OUTPUT "timestamp/bc.htpf.twitter.csv"
  #elif defined(URAND)
    #define OUTPUT "timestamp/bc.htpf.urand.csv"
  #elif defined(ROAD)
    #define OUTPUT "timestamp/bc.htpf.road.csv"
  #elif defined(WEB)
    #define OUTPUT "timestamp/bc.htpf.web.csv"
  #endif 
#elif defined(OMP)
  #if defined(KRON)
    #define OUTPUT "timestamp/bc.homp.kron.csv"
  #elif defined(TWITTER)
    #define OUTPUT "timestamp/bc.homp.twitter.csv"
  #elif defined(URAND)
    #define OUTPUT "timestamp/bc.homp.urand.csv"
  #elif defined(ROAD)
    #define OUTPUT "timestamp/bc.homp.road.csv"
  #elif defined(WEB)
    #define OUTPUT "timestamp/bc.homp.web.csv"
  #endif 
#else
  #if defined(KRON)
    #define OUTPUT "timestamp/bc.kron.csv"
  #elif defined(TWITTER)
    #define OUTPUT "timestamp/bc.twitter.csv"
  #elif defined(URAND)
    #define OUTPUT "timestamp/bc.urand.csv"
  #elif defined(ROAD)
    #define OUTPUT "timestamp/bc.road.csv"
  #elif defined(WEB)
    #define OUTPUT "timestamp/bc.web.csv"
  #endif 
#endif 
#endif 

using namespace std;
typedef float ScoreT;
typedef double CountT;

// static double loop_time; 

// TimeDiff histogram; 
OMPSyncAtomic time_diff(2); 
HyperParam_PfT hyper_param; 

void PrefetchThread1_urand(const SlidingQueue<NodeID>* queue, const Graph* g,
    NodeID* depths, CountT* path_counts, NodeID depth) {
  #if !defined(TUNING)
  HyperParam_PfT hyperparam = {.sync_frequency = 2, .skip_offset = 8, 
                               .serialize_threshold = 50, .unserialize_threshold = 10}; 
  #else
  HyperParam_PfT hyperparam = hyper_param; 
  #endif 
  bool serialize_flag = false; 
  for (auto q_iter = queue->begin(); q_iter < queue->end(); q_iter++) { 
    NodeID u = *q_iter;
    // __builtin_prefetch(q_iter); 
    // __builtin_prefetch(g->out_neigh(*(q_iter++)).end()); 
    for (NodeID &v : g->out_neigh(u)) { // iter per invoc 32 
      // v: 20 cpi, 20%; depths[v] 62 cpi, 58% 
      __builtin_prefetch(&depths[v]); // prefetch depths[v]
      // if (serialize_flag) {
      //   asm volatile ("serialize\n\t"); 
      // }
    }
    if (serialize_flag) {
      asm volatile (
        ".rept 100\n\t" 
        "serialize\n\t" 
        ".endr" 
      );
    }
    sync<NodeID*>(q_iter, (int)sizeof(NodeID), (size_t) q_iter, false, time_diff, 0, ORDER_READ, serialize_flag, hyperparam); 
  }
}

// for kron and twitter 
void PrefetchThread1_inner(const SlidingQueue<NodeID>* queue, const Graph* g,
    NodeID* depths, CountT* path_counts, NodeID depth) {
  #if defined(TWITTER) && !defined(TUNING)
  HyperParam_PfT hyperparam = {.sync_frequency = 80, .skip_offset = 32, 
                                .serialize_threshold = 150, .unserialize_threshold = 140}; 
  #elif defined(KRON) && !defined(TUNING)
  HyperParam_PfT hyperparam = {.sync_frequency = 80, .skip_offset = 32, 
                                .serialize_threshold = 150, .unserialize_threshold = 50}; 
  #else
  HyperParam_PfT hyperparam = hyper_param; 
  #endif 
  bool serialize_flag = false; 
  bool prefetch = true; 
  size_t j = 0; 
  for (auto q_iter = queue->begin(); q_iter < queue->end(); q_iter++) { 
    NodeID u = *q_iter;
    prefetch =  g->out_neigh(u).end() - g->out_neigh(u).begin() > hyperparam.skip_offset ? true : false; 
    for (NodeID* v = g->out_neigh(u).begin(); v < g->out_neigh(u).end(); v++) { 
      if (prefetch) {
        #ifndef ROAD
        if (v + 64 < g->out_neigh(u).end())
          __builtin_prefetch(v+64);
        #else 
        __builtin_prefetch(v);
        #endif 
        __builtin_prefetch(&depths[*v]);
      }
      if (serialize_flag) {
        asm volatile ("serialize\n\t"); 
      }
      /*---inner sync---*/
      if (j % hyperparam.sync_frequency == 0 || serialize_flag) {
        // asm volatile ("serialize\n\t"); 
        size_t main_j = time_diff.read(0, ORDER_READ); 
        // histogram.insert_into_atomic_histogram(main_j, j); 
        if (main_j >= j) {
          serialize_flag = false; 
        } else if (j - main_j > hyperparam.serialize_threshold) {
          serialize_flag = true; 
        } else if (j - main_j < hyperparam.unserialize_threshold) {
          serialize_flag = false; 
        } 
      }
      j++; 
      /*---inner sync---*/
    }
  }
}

void PBFS(const Graph &g, NodeID source, pvector<CountT> &path_counts,
    Bitmap &succ, vector<SlidingQueue<NodeID>::iterator> &depth_index,
    SlidingQueue<NodeID> &queue) {
  pvector<NodeID> depths(g.num_nodes(), -1);
  depths[source] = 0;
  path_counts[source] = 1;
  queue.push_back(source);
  depth_index.push_back(queue.begin());
  queue.slide_window();
  const NodeID* g_out_start = g.out_neigh(0).begin();
  #pragma omp parallel
  {
    NodeID depth = 0;
    QueueBuffer<NodeID> lqueue(queue);
    while (!queue.empty()) {
      depth++;
      // const auto start = chrono::high_resolution_clock::now();
      #if defined(HTPF) && defined(URAND)
      thread PF(PrefetchThread1_urand, &queue, &g, depths.begin(), path_counts.begin(), depth); 
      #elif defined(HTPF) && defined(INNER) && defined(FIRST)
      thread PF(PrefetchThread1_inner, &queue, &g, depths.begin(), path_counts.begin(), depth); 
      time_diff.set(0, 0, ORDER_WRITE); // inner 
      #endif 

      #ifdef BEST
      #pragma omp for schedule(dynamic, 64) nowait
      #endif 
      for (auto q_iter = queue.begin(); q_iter < queue.end(); q_iter++) { 
        #if defined(HTPF) && defined(URAND)
        time_diff.set(0, (size_t) q_iter, ORDER_WRITE); // for urand 
        #endif 
        NodeID u = *q_iter;
        for (NodeID &v : g.out_neigh(u)) { // iter per invoc 32 
          if ((depths[v] == -1) &&
              (compare_and_swap(depths[v], static_cast<NodeID>(-1), depth))) {
            lqueue.push_back(v);
          }
          if (depths[v] == depth) {
            succ.set_bit_atomic(&v - g_out_start);
            #pragma omp atomic
            path_counts[v] += path_counts[u]; 
          }
          #if defined(HTPF) && defined(INNER) && defined(FIRST)
          time_diff.add(0, 1, ORDER_WRITE); // inner 
          #endif // INNER 
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
      }
      // const auto end = chrono::high_resolution_clock::now();
      // loop_time += chrono::duration_cast<chrono::microseconds>(end - start).count(); 
      lqueue.flush();
      #if defined(HTPF) && (defined(FIRST) || defined(URAND))
      PF.join(); 
      #endif 
      #pragma omp barrier
      #pragma omp single
      {
        depth_index.push_back(queue.begin());
        queue.slide_window();
      }
    }
  }
  depth_index.push_back(queue.begin());
}

void PrefetchThread2_urand(const int d, const NodeID* begin, const NodeID* end, 
      const Graph* g, CountT* path_counts, const Bitmap* succ, 
      const NodeID *g_out_start, ScoreT* deltas, ScoreT* scores) {
  #ifndef TUNING
  HyperParam_PfT hyperparam = {.sync_frequency = 0, .skip_offset = 8, 
                                .serialize_threshold = 0, .unserialize_threshold = 0}; 
  #else
  // HyperParam_PfT hyperparam = hyper_param; 
  HyperParam_PfT hyperparam = {.sync_frequency = 0, .skip_offset = 8, 
                                .serialize_threshold = 0, .unserialize_threshold = 0}; 
  #endif
  // uint64_t local_counter = 0; // no sync here
  bool serialize_flag = false; 
  for (auto it = begin; it < end; it++) { // 16777216 iter per invoc 
    NodeID u = *it;
    // local_counter = (size_t) it; 
    for (NodeID &v : g->out_neigh(u)) { // 32 iter per invoc 
      if (succ->get_bit(&v - g_out_start)) {
        __builtin_prefetch(&path_counts[v]);
        __builtin_prefetch(&deltas[v]);
      }
      if (serialize_flag) {
        asm volatile ("serialize\n\t"); 
      }
    }
    __builtin_prefetch(&deltas[u]); // prefetch deltas[u] 
    __builtin_prefetch(&scores[u]); // prefetch scores[u] 

    /*-----sync-----*/
    size_t main_counter = time_diff.read(1, ORDER_READ); 
    it = (NodeID*) (main_counter + hyperparam.skip_offset*sizeof(NodeID)); // always jmp 
    /*-----sync-----*/
  }
}

void PrefetchThread2_inner(const int d, const NodeID* begin, const NodeID* end, 
      const Graph* g, CountT* path_counts, const Bitmap* succ, 
      const NodeID *g_out_start, ScoreT* deltas, ScoreT* scores) {
  #if defined(TWITTER) //&& !defined(TUNING)
  HyperParam_PfT hyperparam = {.sync_frequency = 80, .skip_offset = 64, 
                                .serialize_threshold = 800, .unserialize_threshold = 790}; 
  #elif defined(KRON) //&& !defined(TUNING)
  HyperParam_PfT hyperparam = {.sync_frequency = 80, .skip_offset = 128, 
                                .serialize_threshold = 800, .unserialize_threshold = 770}; 
  #elif defined(ROAD)
  HyperParam_PfT hyperparam = {.sync_frequency = 80, .skip_offset = 128, 
                               .serialize_threshold = 800, .unserialize_threshold = 770}; 
  #elif defined(WEB)
  HyperParam_PfT hyperparam = {.sync_frequency = 200, .skip_offset = 64, 
                               .serialize_threshold = 600, .unserialize_threshold = 590}; 
  #else
  HyperParam_PfT hyperparam = hyper_param; 
  #endif 
  uint64_t j = 0; 
  bool serialize_flag = false; 
  bool prefetch = true; 
  for (auto it = begin; it < end; it++) { 
    NodeID u = *it;
    #ifndef ROAD // filter for kron and twitter
    prefetch = g->out_neigh(u).end() - g->out_neigh(u).begin() > hyperparam.skip_offset ? true : false; 
    #endif 
    #if defined(ROAD) || defined(WEB)
    g->out_neigh(u).prefetch_end(); 
    #endif 
    for (NodeID* v = g->out_neigh(u).begin(); v < g->out_neigh(u).end(); v++) { 
      if (prefetch) {  
        #ifndef ROAD
        if (v +64 < g->out_neigh(u).end())
          __builtin_prefetch(v + 64); 
        #else
        __builtin_prefetch(v); 
        #endif 
      }
      #if defined(KRON) || defined(TWITTER) || defined(WEB) // these are not mem-intensive for web and road 
      if (succ->get_bit(v - g_out_start) && prefetch) { 
        __builtin_prefetch(&path_counts[*v]);
        __builtin_prefetch(&deltas[*v]);
      }
      #endif 
      if (serialize_flag) {
        asm volatile ("serialize\n\t"); 
      }
      j++; 
      /*---inner sync---*/
      if (j % hyperparam.sync_frequency == 0 || serialize_flag) {
        // asm volatile ("serialize\n\t"); 
        size_t main_j = time_diff.read(1, ORDER_READ); 
        // histogram.insert_into_atomic_histogram(main_j, j); 
        if (main_j >= j) {
          serialize_flag = false; 
        } else if (j - main_j > hyperparam.serialize_threshold) {
          serialize_flag = true; 
        } else if (j - main_j < hyperparam.unserialize_threshold) {
          serialize_flag = false; 
        } 
      }
      /*---inner sync---*/
    }
    __builtin_prefetch(&scores[u]); // prefetch scores[u] 
  }
}

pvector<ScoreT> Brandes(const Graph &g, SourcePicker<Graph> &sp,
                        NodeID num_iters, bool logging_enabled = false) {
  Timer t;
  t.Start();
  pvector<ScoreT> scores(g.num_nodes(), 0);
  pvector<CountT> path_counts(g.num_nodes());
  Bitmap succ(g.num_edges_directed());
  vector<SlidingQueue<NodeID>::iterator> depth_index;
  SlidingQueue<NodeID> queue(g.num_nodes());
  t.Stop();
  if (logging_enabled)
    PrintStep("a", t.Seconds());
  const NodeID* g_out_start = g.out_neigh(0).begin();
  for (NodeID iter=0; iter < num_iters; iter++) {
    NodeID source = sp.PickNext();
    if (logging_enabled)
      PrintStep("Source", static_cast<int64_t>(source));
    t.Start();
    path_counts.fill(0);
    depth_index.resize(0);
    queue.reset();
    succ.reset();
    PBFS(g, source, path_counts, succ, depth_index, queue);
    t.Stop();
    if (logging_enabled)
      PrintStep("b", t.Seconds());
    pvector<ScoreT> deltas(g.num_nodes(), 0);
    t.Start();
    for (int d=depth_index.size()-2; d >= 0; d--) {
      #ifdef HTPF
      #ifndef INNER
      thread PF(PrefetchThread2_urand, d, depth_index[d], depth_index[d+1], &g, 
          path_counts.begin(), &succ, g_out_start, deltas.begin(), scores.begin()); 
      #else
      thread PF(PrefetchThread2_inner, d, depth_index[d], depth_index[d+1], &g, 
          path_counts.begin(), &succ, g_out_start, deltas.begin(), scores.begin()); 
      time_diff.set(1, 0, ORDER_WRITE); 
      #endif 
      #endif 
      // const auto start = chrono::high_resolution_clock::now();
      // #pragma omp parallel for schedule(dynamic, 64)
      for (auto it = depth_index[d]; it < depth_index[d+1]; it++) { 
        #if defined(HTPF) && !defined(INNER)
        time_diff.set(1, (size_t) it, ORDER_WRITE); 
        #endif 
        NodeID u = *it;
        ScoreT delta_u = 0;
        for (NodeID &v : g.out_neigh(u)) { // 32 iter per invoc 
          #if defined(HTPF) && defined(INNER)
          time_diff.add(1, 1, ORDER_WRITE); 
          #endif 
          if (succ.get_bit(&v - g_out_start)) {
            delta_u += (path_counts[u] / path_counts[v]) * (1 + deltas[v]); // prefetch path_counts[v] and deltas[v] 
          }
        }
        deltas[u] = delta_u; 
        scores[u] += delta_u; // prefetch scores[u] 
      }
      #ifdef HTPF
      PF.join(); 
      #endif 
    }
    t.Stop();
    if (logging_enabled)
      PrintStep("p", t.Seconds());
  }
  // normalize scores
  ScoreT biggest_score = 0;
  #pragma omp parallel for reduction(max : biggest_score)
  for (NodeID n=0; n < g.num_nodes(); n++)
    biggest_score = max(biggest_score, scores[n]);
  #pragma omp parallel for
  for (NodeID n=0; n < g.num_nodes(); n++)
    scores[n] = scores[n] / biggest_score;
  return scores;
}


void PrintTopScores(const Graph &g, const pvector<ScoreT> &scores) {
  vector<pair<NodeID, ScoreT>> score_pairs(g.num_nodes());
  for (NodeID n : g.vertices())
    score_pairs[n] = make_pair(n, scores[n]);
  int k = 5;
  vector<pair<ScoreT, NodeID>> top_k = TopK(score_pairs, k);
  for (auto kvp : top_k)
    cout << kvp.second << ":" << kvp.first << endl;
}


// Still uses Brandes algorithm, but has the following differences:
// - serial (no need for atomics or dynamic scheduling)
// - uses vector for BFS queue
// - regenerates farthest to closest traversal order from depths
// - regenerates successors from depths
bool BCVerifier(const Graph &g, SourcePicker<Graph> &sp, NodeID num_iters,
                const pvector<ScoreT> &scores_to_test) {
  pvector<ScoreT> scores(g.num_nodes(), 0);
  for (int iter=0; iter < num_iters; iter++) {
    NodeID source = sp.PickNext();
    // BFS phase, only records depth & path_counts
    pvector<int> depths(g.num_nodes(), -1);
    depths[source] = 0;
    vector<CountT> path_counts(g.num_nodes(), 0);
    path_counts[source] = 1;
    vector<NodeID> to_visit;
    to_visit.reserve(g.num_nodes());
    to_visit.push_back(source);
    for (auto it = to_visit.begin(); it != to_visit.end(); it++) {
      NodeID u = *it;
      for (NodeID v : g.out_neigh(u)) {
        if (depths[v] == -1) {
          depths[v] = depths[u] + 1;
          to_visit.push_back(v);
        }
        if (depths[v] == depths[u] + 1)
          path_counts[v] += path_counts[u];
      }
    }
    // Get lists of vertices at each depth
    vector<vector<NodeID>> verts_at_depth;
    for (NodeID n : g.vertices()) {
      if (depths[n] != -1) {
        if (depths[n] >= static_cast<int>(verts_at_depth.size()))
          verts_at_depth.resize(depths[n] + 1);
        verts_at_depth[depths[n]].push_back(n);
      }
    }
    // Going from farthest to closest, compute "dependencies" (deltas)
    pvector<ScoreT> deltas(g.num_nodes(), 0);
    for (int depth=verts_at_depth.size()-1; depth >= 0; depth--) {
      for (NodeID u : verts_at_depth[depth]) {
        for (NodeID v : g.out_neigh(u)) {
          if (depths[v] == depths[u] + 1) {
            deltas[u] += (path_counts[u] / path_counts[v]) * (1 + deltas[v]);
          }
        }
        scores[u] += deltas[u];
      }
    }
  }
  // Normalize scores
  ScoreT biggest_score = *max_element(scores.begin(), scores.end());
  for (NodeID n : g.vertices())
    scores[n] = scores[n] / biggest_score;
  // Compare scores
  bool all_ok = true;
  for (NodeID n : g.vertices()) {
    ScoreT delta = abs(scores_to_test[n] - scores[n]);
    if (delta > std::numeric_limits<ScoreT>::epsilon()) {
      cout << n << ": " << scores[n] << " != " << scores_to_test[n];
      cout << "(" << delta << ")" << endl;
      all_ok = false;
    }
  }
  return all_ok;
}


int main(int argc, char* argv[]) {
  #ifdef OMP
  omp_set_num_threads(2); 
  #endif
  // histogram.init_atomic_histogram(4223223502/2); 
  // loop_time = 0.0; 
  #ifdef TIME
  stamp_counter = 0; 
  array_counter = 0; 
  #endif 

  CLIterApp cli(argc, argv, "betweenness-centrality", 1);
  if (!cli.ParseArgs())
    return -1;
  if (cli.num_iters() > 1 && cli.start_vertex() != -1)
    cout << "Warning: iterating from same source (-r & -i)" << endl;

  hyper_param.sync_frequency = cli.sync_frequency(); 
  hyper_param.skip_offset = cli.skip_offset(); 
  hyper_param.serialize_threshold = cli.serialize_threshold(); 
  hyper_param.unserialize_threshold = cli.serialize_threshold() > cli.unserialize_threshold() ? 
            cli.serialize_threshold() - cli.unserialize_threshold() : 0; 
  cout << "sync frequency = " << hyper_param.sync_frequency << ", serialize threshold = " << hyper_param.serialize_threshold 
       << ", unserialize threshold = " << hyper_param.unserialize_threshold << ", skip offset = " << hyper_param.skip_offset << endl; 
  
  Builder b(cli);
  Graph g = b.MakeGraph();
  SourcePicker<Graph> sp(g, cli.start_vertex());
  auto BCBound = [&sp, &cli] (const Graph &g) {
    return Brandes(g, sp, cli.num_iters(), cli.logging_en());
  };
  SourcePicker<Graph> vsp(g, cli.start_vertex());
  auto VerifierBound = [&vsp, &cli] (const Graph &g,
                                     const pvector<ScoreT> &scores) {
    return BCVerifier(g, vsp, cli.num_iters(), scores);
  };
  #ifdef TIME
  auto kernel_start = chrono::high_resolution_clock::now();
  #endif 
  BenchmarkKernel(cli, g, BCBound, PrintTopScores, VerifierBound);
  // histogram.print_atomic_histogram(); 
  // cout << "loop time = " << loop_time/1e6 << "s" << endl; 
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
