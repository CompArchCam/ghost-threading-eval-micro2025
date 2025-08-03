// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#include <functional>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread> 

#include "omp.h"

#include <sched.h>
#include <pthread.h>

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

#ifndef NT
#define NT 2
#endif 

#ifndef CHUNKSIZE
#define CHUNKSIZE 64
#endif 

#ifndef CHUNKSIZE1
#define CHUNKSIZE1 16384
#endif 

#define HTPF
#define KRON

#if defined(HTPF) && !defined(URAND)
#define INNER
#endif 

// #if defined(KRON) || defined(TWITTER)
// #define FIRST
// #endif 

using namespace std;
typedef float ScoreT;
typedef double CountT;

TimeDiff histogram; 
OMPSyncAtomic time_diff1(NT); 
OMPSyncAtomic time_diff1_outer(NT); 
OMPSyncAtomic time_diff2(NT); 
OMPSyncAtomic end_flag(NT); 
HyperParam_PfT hyper_param; 

void PrefetchThread1_urand(int me, const SlidingQueue<NodeID>* queue, const Graph* g,
    NodeID* depths, CountT* path_counts, NodeID depth, NodeID* start, NodeID* end) {
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
  #if !defined(TUNING)
  HyperParam_PfT hyperparam = {.sync_frequency = 2, .skip_offset = 8, 
                               .serialize_threshold = 50, .unserialize_threshold = 10}; 
  #else
  HyperParam_PfT hyperparam = hyper_param; 
  #endif 
  bool serialize_flag = false; 
  // for (auto q_iter = queue->begin(); q_iter < queue->end(); q_iter++) { 
  for (auto q_iter = start; q_iter < end; q_iter++) { 
    NodeID u = *q_iter;
    for (NodeID &v : g->out_neigh(u)) { 
      __builtin_prefetch(&depths[v]); // prefetch depths[v]
    }
    if (serialize_flag) {
      asm volatile (
        ".rept 100\n\t" 
        "serialize\n\t" 
        ".endr" 
      );
    }
    sync<NodeID*>(q_iter, (int)sizeof(NodeID), (size_t) q_iter, false, time_diff1, me, ORDER_READ, serialize_flag, hyperparam); 
  }
}

#ifdef FIRST
// for kron and twitter 
void PrefetchThread1_inner(int me, NodeID* start, NodeID* end, const SlidingQueue<NodeID>* queue, const Graph* g,
    NodeID* depths, CountT* path_counts, NodeID depth) {
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
  HyperParam_PfT hyperparam = hyper_param; 
  bool serialize_flag = false; 
  bool prefetch = true; 
  size_t j = 0; 
  uint32_t local_counter = 0; 
  for (auto q_iter = start; q_iter < end; q_iter++) { 
    NodeID u = *q_iter;
    prefetch =  g->out_neigh(u).end() - g->out_neigh(u).begin() > hyperparam.skip_offset ? true : false; 
    for (NodeID* v = g->out_neigh(u).begin(); v < g->out_neigh(u).end(); v++) { 
      if (prefetch) {
        // if (v + 64 < g->out_neigh(u).end())
        //   __builtin_prefetch(v+64);
        __builtin_prefetch(&depths[*v]);
      }
      // if ((depths[*v] == -1 || depths[*v] == depth) && prefetch) {
      //   __builtin_prefetch(&path_counts[*v]); 
      // }
      if (serialize_flag) {
        asm volatile ("serialize\n\t"); 
      }
      /*---inner sync---*/
      if (j % hyperparam.sync_frequency == 0 || serialize_flag) {
        // asm volatile ("serialize\n\t"); 
        size_t main_j = time_diff1.read(me, ORDER_READ); 
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
  /*-------sleep stage-------*/
    local_counter++; 
    if (local_counter >= CHUNKSIZE1) {
      while (true) { 
        asm volatile ("serialize\n\t"); 
        if (time_diff1_outer.read(me, ORDER_READ) >= (size_t) q_iter || end_flag.read(me, ORDER_READ)) 
          break; 
      } // wait until the main thread goes to next chunk 
      if (end_flag.read(me, ORDER_READ))
        break; 
      q_iter = (NodeID*) time_diff1_outer.read(me, ORDER_READ); 
      local_counter = 0; 
    }
  /*-------sleep stage-------*/
  }
}
#endif 

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
    NodeID depth = 0;
    QueueBuffer<NodeID> lqueue(queue);
    while (!queue.empty()) {
      depth++;
      #ifdef URAND
      /*-----compute boundary for each pf thread-----*/
      size_t div = (queue.end() -  queue.begin()) / NT; 
      size_t mod = (queue.end() -  queue.begin()) % NT; 
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
      NodeID* start_iter = queue.begin() + head; 
      NodeID* end_iter = queue.begin() + tail; 
      /*-----compute boundary for each pf thread-----*/
      #endif 

      #if defined(HTPF) && !defined(INNER)
      thread PF(PrefetchThread1_urand, me, &queue, &g, depths.begin(), path_counts.begin(), depth, start_iter, end_iter); 
      #elif defined(HTPF) && defined(INNER) && defined(FIRST)
      NodeID* start_iter = queue.begin() + CHUNKSIZE1*me; 
      thread PF(PrefetchThread1_inner, me, start_iter, queue.end(), &queue, &g, depths.begin(), path_counts.begin(), depth); 
      time_diff1.set(me, (size_t) 0, ORDER_WRITE); // inner 
      // time_diff1_outer.set(me, (size_t) start_iter, ORDER_WRITE); 
      end_flag.set(me, 0, ORDER_WRITE); 
      #endif 
      #ifdef URAND
      #pragma omp for schedule(static) nowait
      #else
      #pragma omp for schedule(dynamic, CHUNKSIZE1) nowait
      #endif 
      for (auto q_iter = queue.begin(); q_iter < queue.end(); q_iter++) { 
        #if defined(HTPF) && !defined(INNER)
        time_diff1.set(me, (size_t) q_iter, ORDER_WRITE); // for urand 
        #elif defined(HTPF) && defined(INNER) && defined(FIRST)
        time_diff1_outer.set(me, (size_t) q_iter, ORDER_WRITE); 
        #endif 
        NodeID u = *q_iter;
        for (NodeID &v : g.out_neigh(u)) { 
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
          time_diff1.add(me, 1, ORDER_WRITE); // inner 
          #endif // INNER 
        }
      }
      lqueue.flush();
      #if defined(HTPF) && (defined(FIRST) || defined(URAND))
      end_flag.set(me, 1, ORDER_WRITE); 
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

void PrefetchThread2_urand(int me, const int d, const NodeID* begin, const NodeID* end, 
      const Graph* g, CountT* path_counts, const Bitmap* succ, 
      const NodeID *g_out_start, ScoreT* deltas, ScoreT* scores) {
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
  #ifndef TUNING
  HyperParam_PfT hyperparam = {.sync_frequency = 0, .skip_offset = 9, 
                                .serialize_threshold = 0, .unserialize_threshold = 0}; 
  #else
  // HyperParam_PfT hyperparam = hyper_param; 
  HyperParam_PfT hyperparam = {.sync_frequency = 0, .skip_offset = 9, 
                                .serialize_threshold = 0, .unserialize_threshold = 0}; 
  #endif
  bool serialize_flag = false; 
  for (auto it = begin; it < end; it++) { 
    NodeID u = *it;
    for (NodeID &v : g->out_neigh(u)) { 
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
    size_t main_counter = time_diff2.read(me, ORDER_READ); 
    it = (NodeID*) (main_counter + hyperparam.skip_offset*sizeof(NodeID)); // always jmp 
    /*-----sync-----*/
  }
}

void PrefetchThread2_inner(int me, const int d, const NodeID* begin, const NodeID* end, 
      const Graph* g, CountT* path_counts, const Bitmap* succ, 
      const NodeID *g_out_start, ScoreT* deltas, ScoreT* scores) {
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
  #if defined(TWITTER) && !defined(TUNING)
  HyperParam_PfT hyperparam = {.sync_frequency = 20, .skip_offset = 32, 
                                .serialize_threshold = 600, .unserialize_threshold = 50}; 
  #elif defined(KRON) && !defined(TUNING)
  HyperParam_PfT hyperparam = {.sync_frequency = 20, .skip_offset = 32, 
                                .serialize_threshold = 400, .unserialize_threshold = 70}; 
  #else
  HyperParam_PfT hyperparam = hyper_param; 
  // HyperParam_PfT hyperparam = {.sync_frequency = 20, .skip_offset = 32, 
  //   .serialize_threshold = 400, .unserialize_threshold = 70}; 
  #endif 
  // uint64_t j = 0; 
  uint32_t local_counter = 0; 
  bool serialize_flag = false; 
  bool prefetch = true; 
  begin = (NodeID*) time_diff2.read(me, ORDER_READ); 
  for (auto it = begin; it < end; it++) { 
    local_counter = it - begin; 
    NodeID u = *it;
    prefetch = g->out_neigh(u).end() - g->out_neigh(u).begin() > hyperparam.skip_offset ? true : false; 
    for (NodeID* v = g->out_neigh(u).begin(); v < g->out_neigh(u).end(); v++) { 
      // if (prefetch) {  
      //   if (v +64 < g->out_neigh(u).end())
      //     __builtin_prefetch(v + 64); 
      // }
      #if defined(KRON) || defined(TWITTER) // these are not mem-intensive for web and road 
      if (succ->get_bit(v - g_out_start) && prefetch) { 
        __builtin_prefetch(&path_counts[*v]);
        __builtin_prefetch(&deltas[*v]);
      }
      #endif 
      if (serialize_flag) {
        asm volatile ("serialize\n\t"); 
      }
      /*---inner sync---*/
      // j++; 
      // if (j % hyperparam.sync_frequency == 0 || serialize_flag) {
      //   size_t main_j = time_diff2.read(me, ORDER_READ); 
      //   if (main_j >= j) { // pf thread is too slow 
      //     serialize_flag = false; 
      //   } else if (j - main_j > hyperparam.serialize_threshold) { // too fast 
      //     serialize_flag = true; 
      //   } else if (j - main_j < hyperparam.unserialize_threshold) {
      //     serialize_flag = false; 
      //   } 
      // }
      /*---inner sync---*/
    }
    __builtin_prefetch(&scores[u]); // prefetch scores[u] 
    if (local_counter >= CHUNKSIZE) {
      while (true) { 
        asm volatile ("serialize\n\t"); 
        if (time_diff2.read(me, ORDER_READ) >= (size_t) it || end_flag.read(me, ORDER_READ)) 
          break; 
      } // wait until the main thread goes to next chunk 
      if (end_flag.read(me, ORDER_READ))
        break; 
      it = (NodeID*) time_diff2.read(me, ORDER_READ); 
      begin = it; 
    }
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

        #ifdef URAND
        /*-----compute boundary for each pf thread-----*/
        size_t div = (depth_index[d+1] -  depth_index[d]) / NT; 
        size_t mod = (depth_index[d+1] -  depth_index[d]) % NT; 
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
        NodeID* start_iter = depth_index[d] + head; 
        NodeID* end_iter = depth_index[d] + tail; 
        /*-----compute boundary for each pf thread-----*/
        #endif 

        #ifdef HTPF
        #ifndef INNER
        thread PF(PrefetchThread2_urand, me, d, start_iter, end_iter, &g, 
          path_counts.begin(), &succ, g_out_start, deltas.begin(), scores.begin()); 
        #else
        NodeID* start_iter = depth_index[d] + CHUNKSIZE*me; 
        thread PF(PrefetchThread2_inner, me, d, start_iter, depth_index[d+1], &g, 
            path_counts.begin(), &succ, g_out_start, deltas.begin(), scores.begin()); 
        time_diff2.set(me, (size_t) start_iter, ORDER_WRITE);
        // time_diff2.set(me, 0, ORDER_WRITE); 
        end_flag.set(me, 0, ORDER_WRITE); 
        #endif 
        #endif 
        #ifdef URAND
        #pragma omp for schedule(static) // static scheduling
        #else
        #pragma omp for schedule(dynamic, CHUNKSIZE)
        #endif 
        for (auto it = depth_index[d]; it < depth_index[d+1]; it++) { 
          #if defined(HTPF) && !defined(INNER)
          time_diff2.set(me, (size_t) it, ORDER_WRITE); // urand 
          #elif defined(HTPF) && defined(INNER)
          time_diff2.set(me, (size_t) it, ORDER_WRITE); // iter counter for sleep stage  
          #endif 
          NodeID u = *it;
          ScoreT delta_u = 0;
          for (NodeID &v : g.out_neigh(u)) { 
            // #if defined(HTPF) && defined(INNER)
            // time_diff2.add(me, 1, ORDER_WRITE); 
            // #endif 
            if (succ.get_bit(&v - g_out_start)) {
              delta_u += (path_counts[u] / path_counts[v]) * (1 + deltas[v]); // prefetch path_counts[v] and deltas[v] 
            }
          }
          deltas[u] = delta_u; 
          scores[u] += delta_u; // prefetch scores[u] 
        }
        #ifdef HTPF
        end_flag.set(me, 1, ORDER_WRITE); 
        PF.join(); 
        #endif 
      } // omp parallel 
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
  omp_set_num_threads(NT); 

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
  BenchmarkKernel(cli, g, BCBound, PrintTopScores, VerifierBound);

  return 0;
}
