#pragma once
//#include "malloc_scaled.h"
#include "lib_malloc.h"

//extern TMallocScaled* MemoryAllocator;


#define BLOCK_SIZE_16B   16
#define BLOCK_SIZE_32B   32
#define BLOCK_SIZE_64B   64
#define BLOCK_SIZE_128B  128
#define BLOCK_SIZE_256B  256
#define BLOCK_SIZE_512B  512
#define BLOCK_SIZE_1KB   1024
#define BLOCK_SIZE_2KB   2048
#define BLOCK_SIZE_4KB   4096
#define BLOCK_SIZE_8KB   8192
#define BLOCK_SIZE_16KB  16384
#define BLOCK_SIZE_32KB  32768
#define BLOCK_SIZE_64KB  65536
#define BLOCK_SIZE_128KB 131072
#define BLOCK_SIZE_256KB 262144
#define BLOCK_SIZE_512KB 524288
#define BLOCK_SIZE_1MB   1048576
#define BLOCK_SIZE_2MB   2097152
#define BLOCK_SIZE_4MB   4194304
#define BLOCK_SIZE_8MB   8388608
#define BLOCK_SIZE_16MB  16777216
#define BLOCK_SIZE_32MB  33554432
#define BLOCK_SIZE_64MB  67108864
#define BLOCK_SIZE_128MB 134217728
#define BLOCK_SIZE_256MB 268435456
#define BLOCK_SIZE_512MB 536870912
#define BLOCK_SIZE_1GB   1073741824
#define BLOCK_SIZE_2GB   2147483648
#define BLOCK_SIZE_3GB   3221225472
#define BLOCK_SIZE_4GB   4294967296
#define BLOCK_SIZE_5GB   5368709120
#define BLOCK_SIZE_8GB   8589934592

#define NUM_OF_BLOCKS_10   10
#define NUM_OF_BLOCKS_100  100
#define NUM_OF_BLOCKS_1K   1000
#define NUM_OF_BLOCKS_5K   5000
#define NUM_OF_BLOCKS_10K  10000
#define NUM_OF_BLOCKS_100K 100000
#define NUM_OF_BLOCKS_1M   1000000


void Test_Reserve_And_Release_Area();
void Test_Malloc_And_Free_Block();
void Test_Malloc_Merge_Blocks();
void Test_Area_Overflow();


