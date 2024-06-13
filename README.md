Copyright by Anton Sheludko

# Simple Scaled Memory Allocator

Scaled memory allocator - is a general purpose memory pool allocator.<br />
It allocates, reallocates and releases memory blocks of various sizes in constant time.

The memory allocator was built in and successfuly tested on UNREAL ENGINE 5.4.<br />
Future versions of the malloc will be much more optimized.

# Source structure
 1. source/malloc_scaled      - the malloc source code;
 2. source/test_malloc_scaled - single malloc tests source code;
 3. source/test_mthread_perf_malloc_scaled - multithreaded performance malloc tests sources;
 4. source/malloc_scaled_for_UNREAL_ENGINE_5_4 - malloc source code for UE5;

# How to launch
In your command line run /bin/x64/Release/proj.test_mthread_perf_malloc_scaled.exe --thread-count=N, where N - number of simultaneous testing threads;<br />
By default N=5;<br />
Number of 32 byte blocks allocated by one thread is 10000000; 

# Performance tests
 There are 5 types of tests:
 1. Allocating blocks of constant size: 32 bytes and 64KB
 2. Multiple allocating and releasing blocks
 3. Allocating blocks of increasing size: for ex. from 8 byte till 8MB in arithmetic progression
 4. Tests of small reallocations of blocks;
 5. Tests of big reallocations of blocks;

# Performance tests results
![](/pics/)

Performance test results you may see in the following files:

1. "single_thread_perf_tests.txt"          - time statistics of memory operations performed by single thread
1. "multi_thread_perf_tests_5_threads.txt" - time statistics of memory operations performed by 5 threads






