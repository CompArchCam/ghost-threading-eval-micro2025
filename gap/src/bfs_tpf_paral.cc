// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#include <iostream>
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
#include "pf_support.h"


/*
GAP Benchmark Suite
Kernel: Breadth-First Search (BFS)
Author: Scott Beamer

Will return parent array for a BFS traversal from a source vertex

This BFS implementation makes use of the Direction-Optimizing approach [1].
It uses the alpha and beta parameters to determine whether to switch search
directions. For representing the frontier, it uses a SlidingQueue for the
top-down approach and a Bitmap for the bottom-up approach. To reduce
false-sharing for the top-down approach, thread-local QueueBuffer's are used.

To save time computing the number of edges exiting the frontier, this
implementation precomputes the degrees in bulk at the beginning by storing
them in the parent array as negative numbers. Thus, the encoding of parent is:
  parent[x] < 0 implies x is unvisited and parent[x] = -out_degree(x)
  parent[x] >= 0 implies x been visited

[1] Scott Beamer, Krste Asanović, and David Patterson. "Direction-Optimizing
    Breadth-First Search." International Conference on High Performance
    Computing, Networking, Storage and Analysis (SC), Salt Lake City, Utah,
    November 2012.
*/


#define ORDER_READ memory_order_relaxed
#define ORDER_WRITE memory_order_relaxed

#ifndef NT
#define NT 2
#endif 

#ifndef CHUNKSIZE
#define CHUNKSIZE 1024
#endif 

#define BOUND 5

// #define HTPF
// #define OMP
#define BEST

// #define URAND

#ifdef URAND
#define FIRST
#endif 

#if defined(KRON) || defined(TWITTER) || defined(WEB) || defined(ROAD) 
#define INNER
#endif
 
using namespace std;

TimeDiff histogram; 
OMPSyncAtomic time_diff(NT); 
HyperParam_PfT hyper_param; 

#ifndef BEST
void PrefetchThread1_urand(const Graph *g, NodeID *parent, const Bitmap *front) { 
  #ifdef TUNING
  HyperParam_PfT hyperparam = hyper_param; 
  #else
  // HyperParam_PfT hyperparam = {.sync_frequency = 1, .skip_offset = 24, .serialize_threshold = 90, .unserialize_threshold = 66}; // beijing
  HyperParam_PfT hyperparam = {.sync_frequency = 1, .skip_offset = 13, .serialize_threshold = 60, .unserialize_threshold = 50}; 
  #endif 
  bool serialize_flag = false; 
  for (NodeID u=0; u < g->num_nodes(); u++) { 
    if (parent[u] < 0) {
      for (NodeID *v = g->in_neigh(u).begin(); v < g->in_neigh(u).end(); v++) { 
        if (front->get_bit_then_pf(*v)) { 
          break;
        }
      }
    }
    if (serialize_flag)
      asm volatile (
        ".rept 30\n\t" 
        "serialize\n\t" 
        ".endr" 
      );
    
    sync<NodeID>(u, 1, u, false, time_diff, 0, ORDER_READ, serialize_flag, hyperparam); 
  }
}

#if defined(HTPF) && defined(FIRST)
void PrefetchThread1_kron_twitter(const Graph *g, NodeID *parent, const Bitmap *front) { // 80% coverage 
  // HyperParam_PfT hyper_param_kron_twitter = {.sync_frequency = 20, .skip_offset = 50, 
  //   .serialize_threshold = 200, .unserialize_threshold = 50}; 
  HyperParam_PfT hyper_param_kron_twitter = hyper_param; 
  bool serialize_flag = false; 
  bool prefetch = true; 
  for (NodeID u=0; u < g->num_nodes(); u++) { 
    if (parent[u] < 0) {
      prefetch = g->in_neigh(u).end() - g->in_neigh(u).begin() > 64 ? true : false; 
      for (NodeID *v = g->in_neigh(u).begin(); v < g->in_neigh(u).end(); v++) { 
        if (v +64 < g->in_neigh(u).end() && prefetch)
          __builtin_prefetch(v + 64); 
        if (front->get_bit(*v)) { 
          if (prefetch)
            front->prefetch_bit(*v); 
          break;
        }
      }
    }
    if (serialize_flag)
      asm volatile (
        ".rept 30\n\t" 
        "serialize\n\t" 
        ".endr" 
      );
    sync<NodeID>(u, 1, u, false, time_diff, 0, ORDER_READ, serialize_flag, hyper_param_kron_twitter); 
  }
}
#endif 
#endif // best 

int64_t BUStep(const Graph &g, pvector<NodeID> &parent, Bitmap &front,
               Bitmap &next) { // 80% coverage 
  int64_t awake_count = 0;
  next.reset();
  #if defined(HTPF) && defined(URAND) && !defined(BEST)
  thread PF(PrefetchThread1_urand, &g, parent.begin(), &front); 
  #elif defined(HTPF) && defined(INNER) && defined(FIRST)
  thread PF(PrefetchThread1_kron_twitter, &g, parent.begin(), &front); 
  #endif 
  #pragma omp parallel for num_threads(NT*2) reduction(+ : awake_count) schedule(dynamic, 1024) 
  for (NodeID u=0; u < g.num_nodes(); u++) { 
    #if defined(HTPF) && defined(FIRST) && !defined(BEST)
    time_diff.set(0, (size_t) u, ORDER_WRITE); 
    #endif 
    if (parent[u] < 0) {
      for (NodeID v : g.in_neigh(u)) {
        if (front.get_bit(v)) { 
          parent[u] = v;
          awake_count++;
          next.set_bit(u);
          break;
        }
      }
    }
  }
  #if defined(HTPF) && defined(FIRST) && !defined(BEST)
  PF.join(); // wait PF thread 
  #endif 
  return awake_count;
}

void PrefetchThread2_urand(int me, const SlidingQueue<NodeID> *queue, const Graph *g, 
  const NodeID *parent, NodeID *start, NodeID *end) {
  /*-----pin the pf thread to specfic core-----*/ 
  // pthread_t self = pthread_self();
  // cpu_set_t cpuset;
  // CPU_ZERO(&cpuset);
  // // CPU_SET(me+64, &cpuset); // dublin 
  // CPU_SET((me*2)+1, &cpuset); // beijing 
    
  // int rc = pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpuset);
  // if (rc != 0)
  // {
  //   printf("Failed to pin main thread.\n");
  //   exit(1);
  // }
  /*-----pin the pf thread to specfic core-----*/

  #ifdef TUNING
  HyperParam_PfT hyperparam = hyper_param; 
  #else
  HyperParam_PfT hyperparam = {.sync_frequency = 15, .skip_offset = 5, 
                               .serialize_threshold = 16, .unserialize_threshold = 15}; 
  #endif 
  bool serialize_flag = false; 
  // for (auto q_iter = queue->begin(); q_iter < queue->end(); q_iter++) { 
  for (auto q_iter = start; q_iter < end; q_iter++) { 
    NodeID u = *q_iter;
    for (NodeID *v = g->out_neigh(u).begin(); v < g->out_neigh(u).end(); v++) { 
      __builtin_prefetch(&parent[*v]); 
    }
    if (serialize_flag)
      asm volatile (
        ".rept 50\n\t" 
        "serialize\n\t" 
        ".endr" 
      );
    sync<NodeID*>(q_iter, (int)sizeof(NodeID), (size_t) q_iter, false, time_diff, me, ORDER_READ, serialize_flag, hyperparam); 
  }
}

void PrefetchThread2_kron_twitter(int me, const SlidingQueue<NodeID> *queue, const Graph *g, 
  const NodeID *parent, NodeID *start, NodeID *end) {
  /*-----pin the pf thread to specfic core-----*/ 
  // pthread_t self = pthread_self();
  // cpu_set_t cpuset;
  // CPU_ZERO(&cpuset);
  // // CPU_SET(me+64, &cpuset); // dublin 
  // CPU_SET((me*2)+1, &cpuset); // beijing 
    
  // int rc = pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpuset);
  // if (rc != 0)
  // {
  //   printf("Failed to pin main thread.\n");
  //   exit(1);
  // }
  /*-----pin the pf thread to specfic core-----*/

  #ifdef TUNING
  HyperParam_PfT hyperparam = hyper_param; 
  #else
  // HyperParam_PfT hyper_param_kron_twitter = {.sync_frequency = 1, .skip_offset = 1, 
  //   .serialize_threshold = 10, .unserialize_threshold = 5}; // normal 
  HyperParam_PfT hyperparam = {.sync_frequency = 500, .skip_offset = 128, 
                               .serialize_threshold = 300, .unserialize_threshold = 290}; // membw for kron 
  #endif 
  bool serialize_flag = false; 
  bool prefetch = true; 
  size_t j = 0; 
  for (auto q_iter = start; q_iter < end; q_iter++) { 
    NodeID u = *q_iter;
    prefetch = g->out_neigh(u).end() - g->out_neigh(u).begin() > hyperparam.skip_offset ? true : false; 
    #if defined(ROAD) || defined(WEB)
    g->out_neigh(u).prefetch_end(); 
    #endif 
    for (NodeID *v = g->out_neigh(u).begin(); v < g->out_neigh(u).end(); v++) { 
      if (prefetch) {
        if (v + 64 < g->out_neigh(u).end())
          __builtin_prefetch(v + 64); // only prefetch index in membw hungry condition 
        __builtin_prefetch(&parent[*v]); 
      }
      if (serialize_flag)
        asm volatile (
          ".rept 10\n\t" 
          "serialize\n\t" 
          ".endr" 
        );
        // asm volatile ("serialize\n\t"); 
      /*-----inner sync-----*/
      // if (j % hyperparam.sync_frequency == 0 || serialize_flag) {
      //   size_t main_j = time_diff.read(me, ORDER_READ); 
      //   // histogram.insert_into_atomic_histogram(main_j, j); 
      //   if (main_j >= j) {
      //     serialize_flag = false; 
      //   } else if (j - main_j > hyperparam.serialize_threshold) {
      //     serialize_flag = true; 
      //   } else if (j - main_j < hyperparam.unserialize_threshold) {
      //     serialize_flag = false; 
      //   } 
      // }
      // j++; 
      /*-----inner sync-----*/
    }
  }
}

int64_t TDStep(const Graph &g, pvector<NodeID> &parent,
               SlidingQueue<NodeID> &queue) { // 13% coverage 
  int64_t scout_count = 0;
  #pragma omp parallel num_threads(NT)
  {
    int me = omp_get_thread_num(); 
    /*-----pin the pf thread to specfic core-----*/ 
    // pthread_t self = pthread_self();
    // cpu_set_t cpuset;
    // CPU_ZERO(&cpuset);
    // // CPU_SET(me, &cpuset);
    // CPU_SET(me*2, &cpuset); // beijing
                    
    // int rc = pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpuset);
    // if (rc != 0) {
    //   printf("Failed to pin main thread.\n");
    //   exit(1);
    // }
    /*-----pin the pf thread to specfic core-----*/

    /*-----compute boundary for each pf thread-----*/
    size_t div = (queue.end() - queue.begin()) / NT; 
    size_t mod = (queue.end() - queue.begin()) % NT; 
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
    #ifdef HTPF
    #ifdef URAND
    thread PF(PrefetchThread2_urand, me, &queue, &g, parent.begin(), start_iter, end_iter); 
    #else
    thread PF(PrefetchThread2_kron_twitter, me, &queue, &g, parent.begin(), start_iter, end_iter); 
    // time_diff.set(me, 0, ORDER_WRITE); 
    #endif 
    #endif 
    QueueBuffer<NodeID> lqueue(queue);
    #pragma omp for reduction(+ : scout_count) schedule(static) nowait
    for (auto q_iter = queue.begin(); q_iter < queue.end(); q_iter++) {
      NodeID u = *q_iter;
      #if defined(HTPF) && defined(URAND)
      time_diff.set(me, (size_t) q_iter, ORDER_WRITE); 
      #endif 
      for (NodeID v : g.out_neigh(u)) { 
        NodeID curr_val = parent[v]; 
        if (curr_val < 0) {
          if (compare_and_swap(parent[v], curr_val, u)) {
            lqueue.push_back(v);
            scout_count += -curr_val;
          }
        }
        #if defined(HTPF) && defined(INNER)
        // time_diff.add(me, 1, ORDER_WRITE); 
        #endif 
      }
    }
    lqueue.flush();
    #ifdef HTPF
    PF.join(); 
    #endif 
  }
  return scout_count;
}


void QueueToBitmap(const SlidingQueue<NodeID> &queue, Bitmap &bm) {
  #pragma omp parallel for
  for (auto q_iter = queue.begin(); q_iter < queue.end(); q_iter++) {
    NodeID u = *q_iter;
    bm.set_bit_atomic(u);
  }
}

void BitmapToQueue(const Graph &g, const Bitmap &bm,
                   SlidingQueue<NodeID> &queue) {
  #pragma omp parallel
  {
    QueueBuffer<NodeID> lqueue(queue);
    #pragma omp for nowait
    for (NodeID n=0; n < g.num_nodes(); n++)
      if (bm.get_bit(n))
        lqueue.push_back(n);
    lqueue.flush();
  }
  queue.slide_window();
}

pvector<NodeID> InitParent(const Graph &g) {
  pvector<NodeID> parent(g.num_nodes());
  #pragma omp parallel for
  for (NodeID n=0; n < g.num_nodes(); n++)
    parent[n] = g.out_degree(n) != 0 ? -g.out_degree(n) : -1;
  return parent;
}

pvector<NodeID> DOBFS(const Graph &g, NodeID source, bool logging_enabled = false,
                      int alpha = 15, int beta = 18) {
  if (logging_enabled)
    PrintStep("Source", static_cast<int64_t>(source));
  Timer t;
  t.Start();
  pvector<NodeID> parent = InitParent(g);
  t.Stop();
  if (logging_enabled)
    PrintStep("i", t.Seconds());
  parent[source] = source;
  SlidingQueue<NodeID> queue(g.num_nodes());
  queue.push_back(source);
  queue.slide_window();
  Bitmap curr(g.num_nodes());
  curr.reset();
  Bitmap front(g.num_nodes());
  front.reset();
  int64_t edges_to_check = g.num_edges_directed();
  int64_t scout_count = g.out_degree(source);
  while (!queue.empty()) {
    if (scout_count > edges_to_check / alpha) {
      int64_t awake_count, old_awake_count;
      TIME_OP(t, QueueToBitmap(queue, front));
      if (logging_enabled)
        PrintStep("e", t.Seconds());
      awake_count = queue.size();
      queue.slide_window();
      do {
        t.Start();
        old_awake_count = awake_count;
        // const auto join_start = chrono::high_resolution_clock::now();
        awake_count = BUStep(g, parent, front, curr);
        // const auto join_end = chrono::high_resolution_clock::now();
        // loop_time1 += chrono::duration_cast<chrono::microseconds>(join_end - join_start).count(); 
        front.swap(curr);
        t.Stop();
        if (logging_enabled)
          PrintStep("bu", t.Seconds(), awake_count);
      } while ((awake_count >= old_awake_count) ||
               (awake_count > g.num_nodes() / beta));
      TIME_OP(t, BitmapToQueue(g, front, queue));
      if (logging_enabled)
        PrintStep("c", t.Seconds());
      scout_count = 1;
    } else {
      t.Start();
      edges_to_check -= scout_count;
      // const auto join_start = chrono::high_resolution_clock::now();
      scout_count = TDStep(g, parent, queue);
      // const auto join_end = chrono::high_resolution_clock::now();
      // loop_time2 += chrono::duration_cast<chrono::microseconds>(join_end - join_start).count(); 
      queue.slide_window();
      t.Stop();
      if (logging_enabled)
        PrintStep("td", t.Seconds(), queue.size());
    }
  }
  #pragma omp parallel for
  for (NodeID n = 0; n < g.num_nodes(); n++)
    if (parent[n] < -1)
      parent[n] = -1;
  return parent;
}


void PrintBFSStats(const Graph &g, const pvector<NodeID> &bfs_tree) {
  int64_t tree_size = 0;
  int64_t n_edges = 0;
  for (NodeID n : g.vertices()) {
    if (bfs_tree[n] >= 0) {
      n_edges += g.out_degree(n);
      tree_size++;
    }
  }
  cout << "BFS Tree has " << tree_size << " nodes and ";
  cout << n_edges << " edges" << endl;
}


// BFS verifier does a serial BFS from same source and asserts:
// - parent[source] = source
// - parent[v] = u  =>  depth[v] = depth[u] + 1 (except for source)
// - parent[v] = u  => there is edge from u to v
// - all vertices reachable from source have a parent
bool BFSVerifier(const Graph &g, NodeID source,
                 const pvector<NodeID> &parent) {
  pvector<int> depth(g.num_nodes(), -1);
  depth[source] = 0;
  vector<NodeID> to_visit;
  to_visit.reserve(g.num_nodes());
  to_visit.push_back(source);
  for (auto it = to_visit.begin(); it != to_visit.end(); it++) {
    NodeID u = *it;
    for (NodeID v : g.out_neigh(u)) {
      if (depth[v] == -1) {
        depth[v] = depth[u] + 1;
        to_visit.push_back(v);
      }
    }
  }
  for (NodeID u : g.vertices()) {
    if ((depth[u] != -1) && (parent[u] != -1)) {
      if (u == source) {
        if (!((parent[u] == u) && (depth[u] == 0))) {
          cout << "Source wrong" << endl;
          return false;
        }
        continue;
      }
      bool parent_found = false;
      for (NodeID v : g.in_neigh(u)) {
        if (v == parent[u]) {
          if (depth[v] != depth[u] - 1) {
            cout << "Wrong depths for " << u << " & " << v << endl;
            return false;
          }
          parent_found = true;
          break;
        }
      }
      if (!parent_found) {
        cout << "Couldn't find edge from " << parent[u] << " to " << u << endl;
        return false;
      }
    } else if (depth[u] != parent[u]) {
      cout << "Reachability mismatch" << endl;
      return false;
    }
  }
  return true;
}


int main(int argc, char* argv[]) {
  // omp_set_num_threads(NT); 

  CLApp cli(argc, argv, "breadth-first search");
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
  SourcePicker<Graph> sp(g, cli.start_vertex());
  auto BFSBound = [&sp,&cli] (const Graph &g) {
    return DOBFS(g, sp.PickNext(), cli.logging_en());
  };
  SourcePicker<Graph> vsp(g, cli.start_vertex());
  auto VerifierBound = [&vsp] (const Graph &g, const pvector<NodeID> &parent) {
    return BFSVerifier(g, vsp.PickNext(), parent);
  };
  BenchmarkKernel(cli, g, BFSBound, PrintBFSStats, VerifierBound);

  return 0;
}
