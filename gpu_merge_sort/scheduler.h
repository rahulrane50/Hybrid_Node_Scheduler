/*
 * Noopur Maheshwari : 111464061
 * Rahul Rane : 111465246
 */
#include <iostream>
#include <map>
#include <queue>
#include <functional>
#include <sys/sysinfo.h>
#include "bayes.h"
#define CUDA_KERNEL __global__
using namespace std;

extern map<pthread_t, deque<function<void()>>> cpu_que_map;
extern map<pthread_t, deque<function<void()>>> gpu_que_map;
extern map<pthread_t, cudaStream_t> stream_map;
extern vector<pthread_t> gpu_workers;

template < typename PTHREADID, typename CALLABLE, typename... ARGS >
void submit_task(PTHREADID tid, CALLABLE fn, ARGS&&... args ) { 
    cpu_que_map[tid].push_back( bind( fn, args... ) ) ; 
}


template <typename F, typename ...Args>
void launch_kernel(F fun, Args ...args) {
    pthread_t tid = pthread_self();
    fun<<<1,1>>>(args...);
    cudaDeviceSynchronize();
}

vector<string> __get_testing_data(int p, int q, int r, int *sync_cnt) {
    size_t free_m=0, total_m=0;
    cudaMemGetInfo(&free_m, &total_m);
    float available_gpu = total_m - free_m;
    struct sysinfo info;        
    sysinfo(&info);
    float available_cpu = info.totalram - info.freeram;
    int task_size = r-p;
    //cout<<"task size = "<<task_size<<" avail GPU = "<<available_gpu<<" avail CPU = "<<available_cpu<<endl;
    vector<string> testing_data = {"merge_sort", to_string(task_size), to_string(available_cpu/available_gpu)};
    return testing_data;
}

template <typename FLAG, typename CALLABLE1, typename CALLABLE2, typename... ARGS>
void run_task(FLAG run_flag, CALLABLE1 fn1, CALLABLE2 fn2, ARGS&& ...args) {
    if (run_flag == 0) {
        fn1(args...);
    } else if (run_flag == 1) {
        fn2(args...);
    } else if (run_flag == 2) {
        //decide where to execute
        vector<string> testing_data = __get_testing_data(args...);
        int ret = naive_bayes(testing_data);
        if (ret == 0) {
            fn1(args...);
        } else {
            fn2(args...);
        }
    } 
    else {
        //decide where to execute
        vector<string> testing_data = __get_testing_data(args...);
        int ret = naive_bayes(testing_data);
        if (ret == 0) {
            fn1(args...);
        } else {
            gpu_que_map[gpu_workers[0]].push_back(bind(fn2, args...));
        }
    }
}
