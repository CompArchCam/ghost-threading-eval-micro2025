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

#include <sched.h>
#include <pthread.h>
#include "omp.h"

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

// #define STAMP_ARRAY_SIZE 1216 // multiple of 64 for avoiding false sharing 
// #define VALID_STAMP_SIZE 1200 
#define MAIN_CORE 0
#define PF_CORE 1

#define ORDER_READ memory_order_relaxed
#define ORDER_WRITE memory_order_relaxed

using namespace std;

const WeightT kDistInf = numeric_limits<WeightT>::max()/2;
const size_t kMaxBin = numeric_limits<size_t>::max()/2;
const size_t kBinSizeThreshold = 1000;

// static double loop_time; 

TimeDiff time_diff; 

HyperParam_PfT hyper_param; 

// #define HTPF
// #define INNER 
// #define OMP
// #define TIME

// #define URAND

#if defined(KRON) || defined(TWITTER)
#define INNER
#endif 

#ifdef TIME
#define TOTAL_ITER 4294966740
#define FREQ 1000
uint32_t stamp_counter, array_counter; 
std::chrono::_V2::system_clock::time_point time_array[TOTAL_ITER/FREQ+100]; 

#if defined(HTPF)
  #if defined(KRON)
    #define OUTPUT "timestamp/sssp.htpf.kron.csv"
  #elif defined(TWITTER)
    #define OUTPUT "timestamp/sssp.htpf.twitter.csv"
  #elif defined(URAND)
    #define OUTPUT "timestamp/sssp.htpf.urand.csv"
  #elif defined(ROAD)
    #define OUTPUT "timestamp/sssp.htpf.road.csv"
  #elif defined(WEB)
    #define OUTPUT "timestamp/sssp.htpf.web.csv"
  #endif 
#elif defined(OMP)
  #if defined(KRON)
    #define OUTPUT "timestamp/sssp.homp.kron.csv"
  #elif defined(TWITTER)
    #define OUTPUT "timestamp/sssp.homp.twitter.csv"
  #elif defined(URAND)
    #define OUTPUT "timestamp/sssp.homp.urand.csv"
  #elif defined(ROAD)
    #define OUTPUT "timestamp/sssp.homp.road.csv"
  #elif defined(WEB)
    #define OUTPUT "timestamp/sssp.homp.web.csv"
  #endif 
#else
  #if defined(KRON)
    #define OUTPUT "timestamp/sssp.kron.csv"
  #elif defined(TWITTER)
    #define OUTPUT "timestamp/sssp.twitter.csv"
  #elif defined(URAND)
    #define OUTPUT "timestamp/sssp.urand.csv"
  #elif defined(ROAD)
    #define OUTPUT "timestamp/sssp.road.csv"
  #elif defined(WEB)
    #define OUTPUT "timestamp/sssp.web.csv"
  #endif 
#endif 
#endif 

void PrefetchThread_urand(const WGraph* g, const WeightT* dist, const WeightT delta, 
  const NodeID* frontier, const size_t curr_frontier_tail, const size_t curr_bin_index) 
{
  bool serialize_flag = false; 
  size_t start_iter = time_diff.read_atomic_main(ORDER_READ); //0; 
  for (size_t i=start_iter; i < curr_frontier_tail; i++) {
    NodeID u = frontier[i];
    if (dist[u] >= delta * static_cast<WeightT>(curr_bin_index)) {
      for (WNode* wn = g->out_neigh(u).begin(); wn < g->out_neigh(u).end(); wn++) {
        #if defined(MEMBW)
        if (wn + 64 < g->out_neigh(u).end())
            __builtin_prefetch(wn+64); // prefetch index in membw hungry condition
        #endif 
        __builtin_prefetch(&dist[wn->v]); // prefetch 
      }
    } 
    if (serialize_flag) {
        asm volatile (
          ".rept 115\n\t" 
          "serialize\n\t" 
          ".endr" 
        );
    }
    sync<size_t>(i, 1, i, true, time_diff, ORDER_READ, serialize_flag, hyper_param); 
  }
}

void PrefetchThread_web(const WGraph* g, const WeightT* dist, const WeightT delta, 
  const NodeID* frontier, const size_t curr_frontier_tail, const size_t curr_bin_index) 
{
  #ifdef TUNING
  HyperParam_PfT hyperparam = hyper_param; 
  #else
  HyperParam_PfT hyperparam = {.sync_frequency = 14, .skip_offset = 46, 
                               .serialize_threshold = 100, .unserialize_threshold = 95}; 
  #endif 
  bool serialize_flag = false; 
  size_t start_iter = time_diff.read_atomic_main(ORDER_READ); //0; 
  for (size_t i=start_iter; i < curr_frontier_tail; i++) {
    NodeID u = frontier[i];
    __builtin_prefetch(&frontier[i]); 
    if (dist[u] >= delta * static_cast<WeightT>(curr_bin_index)) {
      g->out_neigh(u).prefetch_begin(); 
      for (WNode wn : g->out_neigh(u)) {
        // prefetching index here do not make a difference
        __builtin_prefetch(&dist[wn.v]); // prefetch 
        // *(volatile WeightT*)&dist[wn.v]; // load 

        if (serialize_flag) {
          asm volatile ("serialize\n\t"); 
        }
      }
    } 
    sync<size_t>(i, 1, i, true, time_diff, ORDER_READ, serialize_flag, hyperparam); 
  }
}

// for kron and twitter 
void PrefetchThread_inner(const WGraph* g, const WeightT* dist, const WeightT delta, 
  const NodeID* frontier, const size_t curr_frontier_tail, const size_t curr_bin_index) 
{
  size_t j = 0; 
  bool serialize_flag = false; 
  for (size_t i=0; i < curr_frontier_tail; i++) {
    NodeID u = frontier[i];
    if (dist[u] >= delta * static_cast<WeightT>(curr_bin_index)) {
      bool prefetch = g->out_neigh(u).end() - g->out_neigh(u).begin() > hyper_param.skip_offset ? true : false; 
      for (WNode* wn = g->out_neigh(u).begin(); wn < g->out_neigh(u).end(); wn++) {
        if (prefetch) {
          #if defined(MEMBW)
          if (wn + 64 < g->out_neigh(u).end())
            __builtin_prefetch(wn+64);
          #endif 
          __builtin_prefetch(&dist[wn->v]); // best for kron and twitter 
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
          size_t main_j = time_diff.read_atomic_main(ORDER_READ); 
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

inline
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
    #if defined(HTPF) && defined(INNER)
    time_diff.add_atomic_main(1, ORDER_WRITE); // for inner sync
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
}

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
    vector<vector<NodeID> > local_bins(0);
    size_t iter = 0;
    while (shared_indexes[iter&1] != kMaxBin) {
      size_t &curr_bin_index = shared_indexes[iter&1];
      size_t &next_bin_index = shared_indexes[(iter+1)&1];
      size_t &curr_frontier_tail = frontier_tails[iter&1];
      size_t &next_frontier_tail = frontier_tails[(iter+1)&1];
      
      #ifdef HTPF
      #ifdef INNER
      thread PF(PrefetchThread_inner, &g, dist.begin(), delta, frontier.begin(), 
                curr_frontier_tail, curr_bin_index); // issue PF thread 
      time_diff.set_atomic_main(0, ORDER_WRITE); 
      #else // not inner 
      #if defined(URAND)
      thread PF(PrefetchThread_urand, &g, dist.begin(), delta, frontier.begin(), 
                curr_frontier_tail, curr_bin_index); // issue PF thread 
      #elif defined(WEB)
      thread PF(PrefetchThread_web, &g, dist.begin(), delta, frontier.begin(), 
                curr_frontier_tail, curr_bin_index); // issue PF thread 
      #endif 
      #endif // INNER
      #endif // HTPF 
      #pragma omp for nowait schedule(dynamic, 64)
      for (size_t i=0; i < curr_frontier_tail; i++) {
        NodeID u = frontier[i];
        if (dist[u] >= delta * static_cast<WeightT>(curr_bin_index)) {
          RelaxEdges(g, u, delta, dist, local_bins);
        } 
        #if defined(HTPF) && !defined(INNER)
        time_diff.set_atomic_main(i, ORDER_WRITE); // for outer sync 
        #endif 
      }
      #ifdef HTPF
      PF.join(); // wait PF thread 
      #endif 

      while (curr_bin_index < local_bins.size() &&
             !local_bins[curr_bin_index].empty() &&
             local_bins[curr_bin_index].size() < kBinSizeThreshold) {
        vector<NodeID> curr_bin_copy = local_bins[curr_bin_index];
        local_bins[curr_bin_index].resize(0);
        // thread PF(PFThread_2, &g, dist.begin(), curr_bin_copy); // issue PF thread
        for (NodeID u : curr_bin_copy)
          RelaxEdges(g, u, delta, dist, local_bins);
        // PF.join(); // wait PF thread
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
  /*-----pin the main thread to specfic core-----*/
  // cout << "main core: #" << MAIN_CORE << ", pf core: #" << PF_CORE << endl; 
  // pthread_t self = pthread_self();
  // cpu_set_t cpuset;
  // CPU_ZERO(&cpuset);
  // CPU_SET(MAIN_CORE, &cpuset);
    
  // int rc = pthread_setaffinity_np(self, sizeof(cpu_set_t), &cpuset);
  // if (rc != 0)
  // {
  //   printf("Failed to pin main thread.\n");
  //   exit(1);
  // }
  /*-----pin the main thread to specfic core-----*/
  #ifdef OMP
  omp_set_num_threads(2); 
  #endif

  #ifdef TIME
  stamp_counter = 0; 
  array_counter = 0; 
  #endif 

  // loop_time = 0.0; 

  time_diff.init_atomic(); 
  // time_diff.init_atomic_histogram(405062551); // 1203

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
  #ifdef TIME
  auto kernel_start = chrono::high_resolution_clock::now();
  #endif 
  BenchmarkKernel(cli, g, SSSPBound, PrintSSSPStats, VerifierBound);
  // cout << "loop time = " << loop_time << "s\n"; 
  
  // print time difference between main and pf_thread 
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
