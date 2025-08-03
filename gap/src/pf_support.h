#ifndef TIMEDIFF_H_
#define TIMEDIFF_H_

#include <iostream>
#include <cstdlib>
#include <chrono>
#include <map>
#include <atomic> 

#define ALIGN_NUM 64

typedef struct SyncStats {
  uint32_t total; 
  uint32_t serialize; 
  uint32_t slow; 
  uint32_t slow_serialize; 
  uint32_t fast; 
  uint32_t fast_serialize; 
  uint32_t normal; 
  uint32_t normal_serialize; 
  size_t padding[4]; // padding to 64 bytes (i.e., one cache line)
} SyncStats __attribute__ ((aligned (64))); 

typedef struct Counter {
  size_t counter; 
  size_t padding[7]; // padding to 64 bytes (i.e., one cache line)
} Counter __attribute__ ((aligned (64))); 

typedef struct Atomic_Counter {
  std::atomic<size_t> counter; 
  size_t padding[7]; // padding to 64 bytes (i.e., one cache line)
} Atomic_Counter __attribute__ ((aligned (64))); 

typedef struct AtomicStamp {
    size_t main; 
    size_t pf; 
} AtomicStamp; 

typedef struct HyperParam_PfT {
  int32_t sync_frequency; 
  int32_t skip_offset; 
  int32_t serialize_threshold; 
  int32_t unserialize_threshold; 
  size_t padding[6]; // padding to 64 bytes (i.e., one cache line)
} HyperParam_PfT __attribute__ ((aligned (64))); 

class TimeDiff {
    public: 
        TimeDiff() : 
            print_histogram_(true),
            interval_us(1000),
            bar_num(25),
            histogram_upper_bound(100),
            atomic_histogram_upper_bound(32),
            atomic_histogram_lower_bound(0)
        { 
            std::cout << "Counter size = " << sizeof(Counter)
                      << ", Atomic Counter size = " << sizeof(Atomic_Counter) 
                      << ", HyperParam size = " << sizeof(HyperParam_PfT)
                      << std::endl; 
        }

        void init(size_t stamp_buffer_size) {
            main_counter.counter = 0; 
            pf_counter.counter = 0; 

            // make sure the size is multiple of ALIGN_NUM 
            stamp_buffer_size_ = (stamp_buffer_size/ALIGN_NUM + 1) * ALIGN_NUM; 
            std::cout << "required size = " << stamp_buffer_size << ", true size = " << stamp_buffer_size_
                      << std::endl; 

            int res1 = posix_memalign((void**)(&main_timestamp), 
                            ALIGN_NUM, 
                            sizeof(std::chrono::_V2::system_clock::time_point)*stamp_buffer_size_); 
            int res2 = posix_memalign((void**)(&pf_timestamp), 
                            ALIGN_NUM, 
                            sizeof(std::chrono::_V2::system_clock::time_point)*stamp_buffer_size_); 
            
            if (res1 || res2) {
                std::cerr << "Error: faile to allocate aligned buffer!\n"; 
            } else {
                std::cout << "init success.";
            }
        }

        void set_histogram_bar_num(int32_t num) {
            bar_num = num; 
        }

        void set_hisgotram_interval(uint32_t interval) {
            interval_us = interval; 
        }

        void set_histogram_upperbound(uint32_t upper_bound) {
            histogram_upper_bound = upper_bound; 
        }

        void insert_timestamp_main() {
            main_timestamp[main_counter.counter] = std::chrono::high_resolution_clock::now(); 
            main_counter.counter++; 
        }

        void insert_timestamp_pf() {
            pf_timestamp[pf_counter.counter] = std::chrono::high_resolution_clock::now();
            pf_counter.counter++; 
        }

        void print() {
            float start_avg, middle_avg, end_avg; 
            if (main_counter.counter != pf_counter.counter) {
                std::cerr << "Error: main and pf thread did different iters!\n"; 
            } else {
                int64_t start_diff_us = 0; 
                int64_t middle_diff_us = 0; 
                int64_t end_diff_us = 0; 
                for (auto i = 0; i < main_counter.counter; i+=3) {
                    auto diff_us = std::chrono::duration_cast<std::chrono::microseconds>(main_timestamp[i] - pf_timestamp[i]).count(); 
                    start_diff_us += diff_us; 
                    diff_us = std::chrono::duration_cast<std::chrono::microseconds>(main_timestamp[i+1] - pf_timestamp[i]).count(); 
                    middle_diff_us += diff_us; 
                    diff_us = std::chrono::duration_cast<std::chrono::microseconds>(main_timestamp[i+2] - pf_timestamp[i]).count(); 
                    end_diff_us += diff_us; 
                }
                start_avg = (float) start_diff_us/(main_counter.counter/3); 
                middle_avg = (float) middle_diff_us/(main_counter.counter/3); 
                end_avg = (float) end_diff_us/(main_counter.counter/3); 
                std::cout << "start: " << start_avg << " us\n"
                          << "middle: " << middle_avg << " us\n"
                          << "end: " << end_avg << " us\nnum of timestamp: " 
                          << main_counter.counter << std::endl; 
            }

            if (print_histogram_) {
                start_histogram.clear(); 
                middle_histogram.clear(); 
                end_histogram.clear(); 
                int64_t start_upper_bound = start_avg + (bar_num/2)*interval_us; 
                int64_t start_lower_bound = start_avg - (bar_num/2)*interval_us; 
                int64_t middle_upper_bound = middle_avg + (bar_num/2)*interval_us; 
                int64_t middle_lower_bound = middle_avg - (bar_num/2)*interval_us; 
                int64_t end_upper_bound = end_avg + (bar_num/2)*interval_us; 
                int64_t end_lower_bound = end_avg - (bar_num/2)*interval_us; 
                for (auto i = 0; i < main_counter.counter; i+=3) {
                    auto diff_us = std::chrono::duration_cast<std::chrono::microseconds>(main_timestamp[i] - pf_timestamp[i]).count(); 
                    if (diff_us > start_upper_bound) {
                        if (start_histogram.count(start_upper_bound/interval_us) == 0)
                            start_histogram[start_upper_bound/interval_us] = 0; 
                        else 
                            start_histogram[start_upper_bound/interval_us]++; 
                    } else if (diff_us < start_lower_bound) {
                        if (start_histogram.count(start_lower_bound/interval_us) == 0)
                            start_histogram[start_lower_bound/interval_us] = 0; 
                        else 
                            start_histogram[start_lower_bound/interval_us]++; 
                    } else {
                        if (start_histogram.count(diff_us/interval_us) == 0)
                            start_histogram[diff_us/interval_us] = 0; 
                        else 
                            start_histogram[diff_us/interval_us]++; 
                    }
                    diff_us = std::chrono::duration_cast<std::chrono::microseconds>(main_timestamp[i+1] - pf_timestamp[i]).count(); 
                    if (diff_us > middle_upper_bound) {
                        if (middle_histogram.count(middle_upper_bound/interval_us) == 0)
                            middle_histogram[middle_upper_bound/interval_us] = 0; 
                        else 
                            middle_histogram[middle_upper_bound/interval_us]++; 
                    } else if (diff_us < middle_lower_bound) {
                        if (middle_histogram.count(middle_lower_bound/interval_us) == 0)
                            middle_histogram[middle_lower_bound/interval_us] = 0; 
                        else 
                            middle_histogram[middle_lower_bound/interval_us]++; 
                    } else {
                        if (middle_histogram.count(diff_us/interval_us) == 0)
                            middle_histogram[diff_us/interval_us] = 0; 
                        else 
                            middle_histogram[diff_us/interval_us]++; 
                    }
                    diff_us = std::chrono::duration_cast<std::chrono::microseconds>(main_timestamp[i+2] - pf_timestamp[i]).count(); 
                    if (diff_us > end_upper_bound) {
                        if (end_histogram.count(end_upper_bound/interval_us) == 0)
                            end_histogram[end_upper_bound/interval_us] = 0; 
                        else 
                            end_histogram[end_upper_bound/interval_us]++; 
                    } else if (diff_us < end_lower_bound) {
                        if (end_histogram.count(end_lower_bound/interval_us) == 0)
                            end_histogram[end_lower_bound/interval_us] = 0; 
                        else 
                            end_histogram[end_lower_bound/interval_us]++; 
                    } else {
                        if (end_histogram.count(diff_us/interval_us) == 0)
                            end_histogram[diff_us/interval_us] = 0; 
                        else 
                            end_histogram[diff_us/interval_us]++; 
                    }
                }
            }
        }

        void print_histogram() {
            std::cout << "start histogram:\n"; 
            for (const auto & iter: start_histogram) {
                std::cout << iter.first*interval_us << ','; 
            }
            std::cout << std::endl; 
            for (const auto & iter: start_histogram) {
                std::cout << iter.second << ','; 
            }
            std::cout << std::endl; 

            std::cout << "middle histogram:\n"; 
            for (const auto & iter: middle_histogram) {
                std::cout << iter.first*interval_us << ','; 
            }
            std::cout << std::endl; 
            for (const auto & iter: middle_histogram) {
                std::cout << iter.second << ','; 
            }
            std::cout << std::endl; 

            std::cout << "end histogram:\n"; 
            for (const auto & iter: end_histogram) {
                std::cout << iter.first*interval_us << ','; 
            }
            std::cout << std::endl; 
            for (const auto & iter: end_histogram) {
                std::cout << iter.second << ','; 
            }
            std::cout << std::endl; 
        }

        void print_all() {
            float avg_ns; 
            if (main_counter.counter != pf_counter.counter) {
                std::cerr << "Error: main and pf thread did different iters!\n"; 
            } else {
                int64_t total_diff_ns = 0; 
                for (auto i = 0; i < main_counter.counter; i++) {
                    auto diff_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(main_timestamp[i] - pf_timestamp[i]).count(); 
                    total_diff_ns += diff_ns; 
                }
                avg_ns = total_diff_ns/main_counter.counter; 
                std::cout << "average time diff: " << avg_ns << " ns\n"
                          << "num of timestamp: " 
                          << main_counter.counter << std::endl; 
            }

            if (print_histogram_) {
                histogram.clear(); 
                int64_t upper_bound = bar_num * interval_us; //histogram_upper_bound; 

                int64_t interval = interval_us; 

                for (auto i = 0; i < main_counter.counter; i++) {
                    auto diff_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(main_timestamp[i] - pf_timestamp[i]).count(); 
                    if (diff_ns < 0) {
                        if (histogram.count(-1) == 0)
                            histogram[-1] = 1; 
                        else 
                            histogram[-1]++; 
                    } else if (diff_ns > upper_bound) {
                        if (histogram.count(upper_bound/interval) == 0)
                            histogram[upper_bound/interval] = 1; 
                        else 
                            histogram[upper_bound/interval]++; 
                    } else {
                        if (histogram.count(diff_ns/interval) == 0)
                            histogram[diff_ns/interval] = 1; 
                        else 
                            histogram[diff_ns/interval]++; 
                    }
                }

                std::cout << "histogram:\n"; 
                for (const auto & iter: histogram) {
                    std::cout << iter.first*interval << ','; 
                }
                std::cout << std::endl; 
                for (const auto & iter: histogram) {
                    std::cout << iter.second << ','; 
                }
                std::cout << std::endl; 
            }
        }

        void init_atomic(size_t main = 0) {
            main_atomic_counter.counter = main; 
            main_atomic_inner_counter.counter = 0; 
            // pf_atomic_counter.counter = pf; 
        }

        void init_atomic_histogram(size_t stamp_buffer_size, int32_t upper = 32, int32_t lower = 0) {
            atomic_histogram_counter.counter = 0; 

            atomic_histogram_upper_bound = upper; 
            atomic_histogram_lower_bound = lower; 

            // make sure the size is multiple of ALIGN_NUM 
            stamp_buffer_size_ = (stamp_buffer_size/ALIGN_NUM + 1) * ALIGN_NUM; 
            std::cout << "required size = " << stamp_buffer_size << ", true size = " << stamp_buffer_size_
                      << std::endl; 

            int res = posix_memalign((void**)(&atomic_stamp_buffer), 
                                     ALIGN_NUM, 
                                     sizeof(AtomicStamp)*stamp_buffer_size_); 
            
            if (res) {
                std::cerr << "Error: faile to allocate aligned buffer!\n"; 
            } else {
                std::cout << "init success.\n";
            }
        }

        void add_atomic_main(size_t i, std::memory_order order) {
            // main_atomic_counter.counter.fetch_add(i, order); 
            main_atomic_counter.counter.store(main_atomic_counter.counter.load(order) + i, order); 
        }

        // void add_atomic_pf(size_t i, std::memory_order order) {
        //     // pf_atomic_counter.counter.fetch_add(i, order); 
        //     pf_atomic_counter.counter.store(pf_atomic_counter.counter.load(order) + i, order); 
        // }

        void set_atomic_main(size_t i, std::memory_order order) {
            main_atomic_counter.counter.store(i, order); 
        }

        size_t read_atomic_main(std::memory_order order) {
            return main_atomic_counter.counter.load(order); 
        }

        void set_atomic_main_inner(size_t i) {
            main_atomic_inner_counter.counter.store(i, std::memory_order_relaxed); 
        }

        size_t read_atomic_main_inner() {
            return main_atomic_inner_counter.counter.load(std::memory_order_relaxed); 
        }

        // size_t read_atomic_pf(std::memory_order order) {
        //     return pf_atomic_counter.counter.load(order); 
        // }

        void insert_into_atomic_histogram(size_t main, size_t pf) {
            atomic_stamp_buffer[atomic_histogram_counter.counter].main = main; 
            atomic_stamp_buffer[atomic_histogram_counter.counter].pf = pf; 
            atomic_histogram_counter.counter++; 
        }

        void print_atomic_histogram() {
            for (int i = atomic_histogram_lower_bound; i <= atomic_histogram_upper_bound; i++) {
                atomic_histogram[i] = 0; 
            }
            atomic_histogram[atomic_histogram_lower_bound-1] = 0; 
            atomic_histogram[atomic_histogram_upper_bound+1] = 0; 

            for (size_t i = 0; i < atomic_histogram_counter.counter; i++) {
                if ((int64_t)atomic_stamp_buffer[i].pf - 
                    (int64_t)atomic_stamp_buffer[i].main < atomic_histogram_lower_bound) 
                { // diff < lower 
                    atomic_histogram[atomic_histogram_lower_bound-1]++; 
                } else if ((int64_t)atomic_stamp_buffer[i].pf - 
                           (int64_t)atomic_stamp_buffer[i].main > atomic_histogram_upper_bound) 
                { // diff > upper 
                    atomic_histogram[atomic_histogram_upper_bound+1]++; 
                } else { // lower <= diff <= lower
                    atomic_histogram[(int64_t)atomic_stamp_buffer[i].pf - (int64_t)atomic_stamp_buffer[i].main]++; 
                }
            }

            std::cout << "number of sample points = " << atomic_histogram_counter.counter << std::endl; 
            // for (const auto& iter: atomic_histogram) {
            //     std::cout << iter.first << ','; 
            // }
            std::cout << std::endl; 
            for (const auto& iter: atomic_histogram) {
                std::cout << iter.second << '\n'; 
            }
            // std::cout << std::endl; 
        }
        ~TimeDiff() {
            delete [] main_timestamp; 
            delete [] pf_timestamp; 
            delete [] atomic_stamp_buffer; 
        }

    private: 
        size_t stamp_buffer_size_; 
        Counter main_counter; 
        Counter pf_counter; 
        std::chrono::_V2::system_clock::time_point* main_timestamp; 
        std::chrono::_V2::system_clock::time_point* pf_timestamp; 

        bool print_histogram_; 

        std::map<int64_t, int32_t> start_histogram; 
        std::map<int64_t, int32_t> middle_histogram; 
        std::map<int64_t, int32_t> end_histogram; 

        std::map<int64_t, int32_t> histogram; 

        uint32_t interval_us; 
        uint32_t bar_num; // odd value 
        uint32_t histogram_upper_bound; 

        Atomic_Counter main_atomic_counter; 
        Atomic_Counter main_atomic_inner_counter; 
        // Atomic_Counter pf_atomic_counter; 

        AtomicStamp* atomic_stamp_buffer; 
        std::map<int32_t, uint32_t> atomic_histogram; 

        Counter atomic_histogram_counter; 
        int32_t atomic_histogram_upper_bound; 
        int32_t atomic_histogram_lower_bound; 
}; 

class OMPSyncAtomic {
    public: 
        OMPSyncAtomic(int thread_num) : 
            thread_num_(thread_num)
        {
            // main_atomic_counter = new Atomic_Counter[thread_num_]; 
            int res = posix_memalign((void**)(&main_atomic_counter), 
                                     ALIGN_NUM, 
                                     sizeof(Atomic_Counter)*thread_num_); 
            
            if (res) {
                std::cerr << "Error: faile to allocate aligned buffer!\n"; 
            } else {
                std::cout << "init success.";
            }

            std::cout << "number of threads = " << thread_num_ << ", size of counter = " 
                      << sizeof(Atomic_Counter) << std::endl; 
        }

        void set(int tid, size_t i, std::memory_order order) {
            main_atomic_counter[tid].counter.store(i, order); 
        }

        void add(int tid, size_t i, std::memory_order order) {
            // main_atomic_counter.counter.fetch_add(i, order); 
            main_atomic_counter[tid].counter.store(main_atomic_counter[tid].counter.load(order) + i, order); 
        }

        size_t read(int tid, std::memory_order order) {
            return main_atomic_counter[tid].counter.load(order); 
        }

        ~OMPSyncAtomic() {
            delete [] main_atomic_counter; 
        }

    private: 
        Atomic_Counter *main_atomic_counter; 
        int thread_num_; 
        // Atomic_Counter pf_atomic_counter; 
}; 

template <typename T> inline void sync(T &iterator, int stride, size_t i, 
    bool accurate_sync, TimeDiff &time_diff, std::memory_order read_order,
    bool &serialize_flag, const HyperParam_PfT &hyper_param) {
    if (i % hyper_param.sync_frequency == 0 || serialize_flag) { // when serialize enabled, check more often 
        if (accurate_sync)
            asm volatile ("serialize\n\t"); // a serialize here to make sure the counter is most up-to-date 
        size_t main_counter = time_diff.read_atomic_main(read_order); 
        // time_diff.insert_into_atomic_histogram(main_counter, i); 
        if (main_counter >= i) { // if pf thread is too slow 
            serialize_flag = false; 
            iterator = (T) (main_counter + hyper_param.skip_offset*stride); 
        } else if (i - main_counter > hyper_param.serialize_threshold) { // pf thread is too fast 
            serialize_flag = true; 
        } else if (i - main_counter < hyper_param.unserialize_threshold) {
            serialize_flag = false; 
        }
    }
}

template <typename T> inline bool sync_jump(T &iterator, int stride, size_t i, 
    bool accurate_sync, TimeDiff &time_diff, std::memory_order read_order,
    bool &serialize_flag, const HyperParam_PfT &hyper_param) {
    if (i % hyper_param.sync_frequency == 0 || serialize_flag) { // when serialize enabled, check more often 
        if (accurate_sync)
            asm volatile ("serialize\n\t"); // a serialize here to make sure the counter is most up-to-date 
        size_t main_counter = time_diff.read_atomic_main(read_order); 
        if (main_counter >= i) { // if pf thread is too slow 
            serialize_flag = false; 
            iterator = (T) (main_counter + hyper_param.skip_offset*stride); 
            return true; 
        } else if (i - main_counter > hyper_param.serialize_threshold) { // pf thread is too fast 
            serialize_flag = true; 
        } else if (i - main_counter < hyper_param.unserialize_threshold) {
            serialize_flag = false; 
        }
    }
    return false; 
}

template <typename T> inline void sync(T &iterator, int stride, size_t i, 
    bool accurate_sync, OMPSyncAtomic &time_diff, int tid, std::memory_order read_order,
    bool &serialize_flag, const HyperParam_PfT &hyper_param) {
    if (i % hyper_param.sync_frequency == 0 || serialize_flag) { // when serialize enabled, check more often 
        if (accurate_sync)
            asm volatile ("serialize\n\t"); // a serialize here to make sure the counter is most up-to-date 
        size_t main_counter = time_diff.read(tid, read_order); 
        // time_diff.insert_into_atomic_histogram(main_counter, local_counter); 
        if (main_counter >= i) { // if pf thread is too slow 
            serialize_flag = false; 
            iterator = (T) (main_counter + hyper_param.skip_offset*stride); 
        } else if (i - main_counter > hyper_param.serialize_threshold) { // pf thread is too fast 
            serialize_flag = true; 
        } else if (i - main_counter < hyper_param.unserialize_threshold) {
            serialize_flag = false; 
        }
    }
}

#endif // TIMEDIFF_H_