Copyright by Anton Sheludko

# Simple Scaled Memory Allocator

Scaled memory allocator - is a general purpose memory pool allocator.
It allocates, reallocates and releases memory blocks of various sizes in constant time.

The memory allocator was built in and successfuly tested on UNREAL ENGINE 5.4.
Future versions of the malloc will be much more optimized.

# Source structure
 1. source/malloc_scaled      - the malloc source code;
 2. source/test_malloc_scaled - single malloc tests source code;
 3. source/test_mthread_perf_malloc_scaled - multithreaded performance malloc tests sources;
 4. source/malloc_scaled_for_UNREAL_ENGINE_5_4 - malloc source code for UE5;

# How to launch
Launch /bin/x64/Release/proj.test_mthread_perf_malloc_scaled.exe --thread-count=N, where N - number of simultaneous testing threads;
By default N=5;

# Performance tests results


Performance test results you may see in the following files:

1. "single_thread_perf_tests.txt"          - time statistics of memory operations performed by single thread
1. "multi_thread_perf_tests_5_threads.txt" - time statistics of memory operations performed by 5 threads






