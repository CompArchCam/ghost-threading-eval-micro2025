// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#include <cinttypes>
#include <limits>
#include <iostream>
#include <queue>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdlib>

#include <omp.h>

#include <sched.h>
#include <pthread.h>

#include "benchmark.h"
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "platform_atomics.h"
#include "pvector.h"
#include "timer.h"

// #include "ctpl_stl.h"
#include "pf_support.h"


/*
GAP Benchmark Suite
Kernel: Single-source Shortest Paths (SSSP)
Author: Scott Beamer, Yunming Zhang

Returns array of distances for all vertices from given source vertex

This SSSP implementation makes use of the ∆-stepping algorithm [1]. The type
used for weights and distances (WeightT) is typedefined in benchmark.h. The
delta parameter (-d) should be set for each input graph. This implementation
incorporates a new bucket fusion optimization [2] that significantly reduces
the number of iterations (& barriers) needed.

The bins of width delta are actually all thread-local and of type std::vector,
so they can grow but are otherwise capacity-proportional. Each iteration is
done in two phases separated by barriers. In the first phase, the current
shared bin is processed by all threads. As they find vertices whose distance
they are able to improve, they add them to their thread-local bins. During this
phase, each thread also votes on what the next bin should be (smallest
non-empty bin). In the next phase, each thread copies its selected
thread-local bin into the shared bin.

Once a vertex is added to a bin, it is not removed, even if its distance is
later updated and, it now appears in a lower bin. We find ignoring vertices if
their distance is less than the min distance for the current bin removes
enough redundant work to be faster than removing the vertex from older bins.

The bucket fusion optimization [2] executes the next thread-local bin in
the same iteration if the vertices in the next thread-local bin have the
same priority as those in the current shared bin. This optimization greatly
reduces the number of iterations needed without violating the priority-based
execution order, leading to significant speedup on large diameter road networks.

[1] Ulrich Meyer and Peter Sanders. "δ-stepping: a parallelizable shortest path
    algorithm." Journal of Algorithms, 49(1):114–152, 2003.

[2] Yunming Zhang, Ajay Brahmakshatriya, Xinyi Chen, Laxman Dhulipala,
    Shoaib Kamil, Saman Amarasinghe, and Julian Shun. "Optimizing ordered graph
    algorithms with GraphIt." The 18th International Symposium on Code Generation
    and Optimization (CGO), pages 158-170, 2020.
*/

#ifndef NT
#define NT 2
#endif 

#if defined(KRON) || defined(TWITTER)
#define INNER
#endif

#define ORDER_READ memory_order_relaxed
#define ORDER_WRITE memory_order_relaxed

using namespace std;

const WeightT kDistInf = numeric_limits<WeightT>::max()/2;
const size_t kMaxBin = numeric_limits<size_t>::max()/2;
const size_t kBinSizeThreshold = 1000;

int pf_core; 
int pf_dist; 

static uint64_t setup_time; 
static uint64_t wait_join_time; 

OMPSyncAtomic time_diff(NT); 

HyperParam_PfT hyper_param; 

void PrefetchThread_urand_paral(int me, const WGraph* g, const WeightT* dist, const WeightT delta, 
  const NodeID* frontier, const size_t curr_frontier_tail, const size_t curr_bin_index) 
{
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

  size_t local_counter = 0; 
  bool serialize_flag = false; 
  size_t start_iter = time_diff.read(me, ORDER_READ); 
  for (size_t i=start_iter; i < curr_frontier_tail; i++) {
    NodeID u = frontier[i];
    local_counter = i; 
    if (dist[u] >= delta * static_cast<WeightT>(curr_bin_index)) {
      for (WNode* wn = g->out_neigh(u).begin(); wn < g->out_neigh(u).end(); wn++) {
        // if (wn + 64 < g->out_neigh(u).end())
          // __builtin_prefetch(wn+64);
        __builtin_prefetch(&dist[wn->v], 0, 3); 
      }
    } 
    if (serialize_flag) {
        asm volatile (
          ".rept 150\n\t" 
          // ".rept 115\n\t" 
          "serialize\n\t" 
          ".endr" 
        );
    }
    sync<size_t>(i, 1, i, true, time_diff, me, ORDER_READ, serialize_flag, hyper_param); 
  }
}

void PrefetchThread_inner_paral(int me, const WGraph* g, const WeightT* dist, const WeightT delta, 
  const NodeID* frontier, const size_t start, const size_t curr_frontier_tail, 
  const size_t curr_bin_index) 
{
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

  size_t j = 0; 
  bool serialize_flag = false; 
  for (size_t i=start; i < curr_frontier_tail; i++) {
    NodeID u = frontier[i];
    if (dist[u] >= delta * static_cast<WeightT>(curr_bin_index)) {
      bool prefetch = g->out_neigh(u).end() - g->out_neigh(u).begin() > hyper_param.skip_offset ? true : false; 
      for (WNode* wn = g->out_neigh(u).begin(); wn < g->out_neigh(u).end(); wn++) {
        if (prefetch) {
          // if (wn + 64 < g->out_neigh(u).end())
          //   __builtin_prefetch(wn+64, 0, 2);
          __builtin_prefetch(&dist[wn->v], 0, 2); // best for kron and twitter 
        }
        if (serialize_flag) {
          asm volatile (
            ".rept 1\n\t" 
            "serialize\n\t" 
            ".endr" 
          );
        }
        /*---inner sync---*/
        if (j % hyper_param.sync_frequency == 0 || serialize_flag) {
          // asm volatile ("serialize\n\t"); 
          size_t main_j = time_diff.read(me, ORDER_READ); 
          // time_diff.insert_into_atomic_histogram(main_j, j); 
          if (main_j >= j) {
            serialize_flag = false; 
          } else if (j - main_j > hyper_param.serialize_threshold) {
            serialize_flag = true; 
          } else if (j - main_j < hyper_param.unserialize_threshold) {
            serialize_flag = false; 
          } 
        }
        /*---inner sync---*/
        j++; 
      } // inner loop 
    } // if 
  } // outer loop 
}

void PrefetchThread_web_paral(int me, const WGraph* g, const WeightT* dist, const WeightT delta, 
  const NodeID* frontier, const size_t curr_frontier_tail, const size_t curr_bin_index) 
{
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
  bool serialize_flag = false; 
  size_t start_iter = time_diff.read(me, ORDER_READ); 
  for (size_t i=start_iter; i < curr_frontier_tail; i++) {
    NodeID u = frontier[i];
    __builtin_prefetch(&frontier[i]); 
    if (dist[u] >= delta * static_cast<WeightT>(curr_bin_index)) {
      g->out_neigh(u).prefetch_begin(); 
      for (WNode wn : g->out_neigh(u)) {
        // __builtin_prefetch(&wn); 
        __builtin_prefetch(&dist[wn.v]); // prefetch 

        if (serialize_flag) {
          asm volatile ("serialize\n\t"); 
        }
      }
    } 
    sync<size_t>(i, 1, i, true, time_diff, me, ORDER_READ, serialize_flag, hyper_param); 
  }
}

#ifdef INNER
inline
void RelaxEdges(int me, const WGraph &g, NodeID u, WeightT delta,
                pvector<WeightT> &dist, vector <vector<NodeID>> &local_bins) {
  for (WNode wn : g.out_neigh(u)) { 
    WeightT old_dist = dist[wn.v];
    WeightT new_dist = dist[u] + wn.w;
    while (new_dist < old_dist) {
      if (compare_and_swap(dist[wn.v], old_dist, new_dist)) { // modify dist
        size_t dest_bin = new_dist/delta;
        if (dest_bin >= local_bins.size())
          local_bins.resize(dest_bin+1);
        local_bins[dest_bin].push_back(wn.v);
        break;
      }
      old_dist = dist[wn.v];      // swap failed, recheck dist update & retry
    }
    #ifdef INNER
    time_diff.add(me, 1, ORDER_WRITE); 
    #endif 
  }
}
#else
void RelaxEdges(const WGraph &g, NodeID u, WeightT delta,
                pvector<WeightT> &dist, vector <vector<NodeID>> &local_bins) {
  for (WNode wn : g.out_neigh(u)) { 
    WeightT old_dist = dist[wn.v];
    WeightT new_dist = dist[u] + wn.w;
    while (new_dist < old_dist) {
      if (compare_and_swap(dist[wn.v], old_dist, new_dist)) { // modify dist
        size_t dest_bin = new_dist/delta;
        if (dest_bin >= local_bins.size())
          local_bins.resize(dest_bin+1);
        local_bins[dest_bin].push_back(wn.v);
        break;
      }
      old_dist = dist[wn.v];      // swap failed, recheck dist update & retry
    }
  }
}
#endif 

pvector<WeightT> DeltaStep(const WGraph &g, NodeID source, WeightT delta,
                           bool logging_enabled = false) {
  Timer t;
  pvector<WeightT> dist(g.num_nodes(), kDistInf);
  dist[source] = 0;
  pvector<NodeID> frontier(g.num_edges_directed());
  // two element arrays for double buffering curr=iter&1, next=(iter+1)&1
  size_t shared_indexes[2] = {0, kMaxBin};
  size_t frontier_tails[2] = {1, 0};
  frontier[0] = source;
  t.Start();
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
    vector<vector<NodeID> > local_bins(0);
    size_t iter = 0;
    while (shared_indexes[iter&1] != kMaxBin) {
      size_t &curr_bin_index = shared_indexes[iter&1];
      size_t &next_bin_index = shared_indexes[(iter+1)&1];
      size_t &curr_frontier_tail = frontier_tails[iter&1];
      size_t &next_frontier_tail = frontier_tails[(iter+1)&1];
      
      /*-----compute boundary for each pf thread-----*/
      size_t div = curr_frontier_tail / NT; 
      size_t mod = curr_frontier_tail % NT; 
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
      /*-----compute boundary for each pf thread-----*/
      #if defined(URAND)
      time_diff.set(me, head, ORDER_WRITE); 
      thread PF(PrefetchThread_urand_paral, me, &g, dist.begin(), delta, frontier.begin(), 
                tail, curr_bin_index); // issue PF thread 
      #elif defined(WEB)
      time_diff.set(me, head, ORDER_WRITE); 
      thread PF(PrefetchThread_web_paral, me, &g, dist.begin(), delta, frontier.begin(), 
                tail, curr_bin_index); // issue PF thread 
      #elif defined(INNER) 
      thread PF(PrefetchThread_inner_paral, me, &g, dist.begin(), delta, frontier.begin(), 
                head, tail, curr_bin_index); // issue PF thread 
      time_diff.set(me, 0, ORDER_WRITE); 
      #endif 
      #pragma omp for nowait schedule(static) // schedule(dynamic, 64) 
      for (size_t i=0; i < curr_frontier_tail; i++) {
        NodeID u = frontier[i];
        if (dist[u] >= delta * static_cast<WeightT>(curr_bin_index)) {
          #ifdef INNER
          RelaxEdges(me, g, u, delta, dist, local_bins);
          #else
          RelaxEdges(g, u, delta, dist, local_bins);
          #endif
        } 
        #if !defined(INNER) 
        time_diff.set(me, i, ORDER_WRITE); // inner sync 
        #endif 
      }
      #if defined(URAND) || defined(WEB) || defined(INNER)
      PF.join(); // wait PF thread 
      #endif 

      while (curr_bin_index < local_bins.size() &&
             !local_bins[curr_bin_index].empty() &&
             local_bins[curr_bin_index].size() < kBinSizeThreshold) {
        vector<NodeID> curr_bin_copy = local_bins[curr_bin_index];
        local_bins[curr_bin_index].resize(0);
        for (NodeID u : curr_bin_copy) {
          #ifdef INNER
          RelaxEdges(me, g, u, delta, dist, local_bins);
          #else
          RelaxEdges(g, u, delta, dist, local_bins);
          #endif
        }
      }
      for (size_t i=curr_bin_index; i < local_bins.size(); i++) {
        if (!local_bins[i].empty()) {
          #pragma omp critical
          next_bin_index = min(next_bin_index, i);
          break;
        }
      }
      #pragma omp barrier
      #pragma omp single nowait
      {
        t.Stop();
        if (logging_enabled)
          PrintStep(curr_bin_index, t.Millisecs(), curr_frontier_tail);
        t.Start();
        curr_bin_index = kMaxBin;
        curr_frontier_tail = 0;
      }
      if (next_bin_index < local_bins.size()) {
        size_t copy_start = fetch_and_add(next_frontier_tail,
                                          local_bins[next_bin_index].size());
        copy(local_bins[next_bin_index].begin(),
             local_bins[next_bin_index].end(), frontier.data() + copy_start);
        local_bins[next_bin_index].resize(0);
      }
      iter++;
      #pragma omp barrier
    }
    #pragma omp single
    if (logging_enabled)
      cout << "took " << iter << " iterations" << endl;
  }
  return dist;
}


void PrintSSSPStats(const WGraph &g, const pvector<WeightT> &dist) {
  auto NotInf = [](WeightT d) { return d != kDistInf; };
  int64_t num_reached = count_if(dist.begin(), dist.end(), NotInf);
  cout << "SSSP Tree reaches " << num_reached << " nodes" << endl;
}


// Compares against simple serial implementation
bool SSSPVerifier(const WGraph &g, NodeID source,
                  const pvector<WeightT> &dist_to_test) {
  // Serial Dijkstra implementation to get oracle distances
  pvector<WeightT> oracle_dist(g.num_nodes(), kDistInf);
  oracle_dist[source] = 0;
  typedef pair<WeightT, NodeID> WN;
  priority_queue<WN, vector<WN>, greater<WN>> mq;
  mq.push(make_pair(0, source));
  while (!mq.empty()) {
    WeightT td = mq.top().first;
    NodeID u = mq.top().second;
    mq.pop();
    if (td == oracle_dist[u]) {
      for (WNode wn : g.out_neigh(u)) {
        if (td + wn.w < oracle_dist[wn.v]) {
          oracle_dist[wn.v] = td + wn.w;
          mq.push(make_pair(td + wn.w, wn.v));
        }
      }
    }
  }
  // Report any mismatches
  bool all_ok = true;
  for (NodeID n : g.vertices()) {
    if (dist_to_test[n] != oracle_dist[n]) {
      cout << n << ": " << dist_to_test[n] << " != " << oracle_dist[n] << endl;
      all_ok = false;
    }
  }
  return all_ok;
}


int main(int argc, char* argv[]) {
  omp_set_num_threads(NT); // set num_threads by pf_dist 

  setup_time = 0; 
  wait_join_time = 0; 

  printf("main thread Running: CPU %d\n", sched_getcpu());
  CLDelta<WeightT> cli(argc, argv, "single-source shortest-path");
  if (!cli.ParseArgs())
    return -1;
  hyper_param.sync_frequency = cli.sync_frequency(); 
  hyper_param.skip_offset = cli.skip_offset(); 
  hyper_param.serialize_threshold = cli.serialize_threshold(); 
  hyper_param.unserialize_threshold = cli.serialize_threshold() > cli.unserialize_threshold() ? 
            cli.serialize_threshold() - cli.unserialize_threshold() : 0; 
  cout << "sync frequency = " << hyper_param.sync_frequency << ", serialize threshold = " << hyper_param.serialize_threshold 
       << ", unserialize threshold = " << hyper_param.unserialize_threshold << ", skip offset = " << hyper_param.skip_offset << endl; 
  WeightedBuilder b(cli);
  WGraph g = b.MakeGraph();
  SourcePicker<WGraph> sp(g, cli.start_vertex());
  auto SSSPBound = [&sp, &cli] (const WGraph &g) {
    return DeltaStep(g, sp.PickNext(), cli.delta(), cli.logging_en());
  };
  SourcePicker<WGraph> vsp(g, cli.start_vertex());
  auto VerifierBound = [&vsp] (const WGraph &g, const pvector<WeightT> &dist) {
    return SSSPVerifier(g, vsp.PickNext(), dist);
  };
  BenchmarkKernel(cli, g, SSSPBound, PrintSSSPStats, VerifierBound);
  cout << "thread setup time = " << setup_time << "us, thread join time = " << wait_join_time << "us\n"; 

  return 0;
}
