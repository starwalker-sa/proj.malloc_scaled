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

#define BLOCK_SIZE_4B          4
#define BLOCK_SIZE_19B         19
#define BLOCK_SIZE_33B         33
#define BLOCK_SIZE_77B         77
#define BLOCK_SIZE_111B        111
#define BLOCK_SIZE_444B        444
#define BLOCK_SIZE_700B        700
#define BLOCK_SIZE_1500B       1500
#define BLOCK_SIZE_2500B       2500
#define BLOCK_SIZE_5000B       5000
#define BLOCK_SIZE_12000B      12000
#define BLOCK_SIZE_20000B      20000
#define BLOCK_SIZE_40000B      40000
#define BLOCK_SIZE_110000B     110000
#define BLOCK_SIZE_200000B     200000
#define BLOCK_SIZE_700000B     700000
#define BLOCK_SIZE_1500000B    1500000
#define BLOCK_SIZE_3000000B    3000000
#define BLOCK_SIZE_7000000B    7000000
#define BLOCK_SIZE_12000000B   12000000
#define BLOCK_SIZE_20000000B   20000000
#define BLOCK_SIZE_40000000B   40000000
#define BLOCK_SIZE_80000000B   80000000
#define BLOCK_SIZE_180000000B  180000000
#define BLOCK_SIZE_300000000B  300000000
#define BLOCK_SIZE_800000000B  800000000
#define BLOCK_SIZE_2000000000B 2000000000
#define BLOCK_SIZE_3000000000B 3000000000
#define BLOCK_SIZE_4000000000B 4000000000
#define BLOCK_SIZE_5000000000B 5000000000
#define BLOCK_SIZE_8000000000B 8000000000

#define NUM_OF_BLOCKS_10   10
#define NUM_OF_BLOCKS_100  100
#define NUM_OF_BLOCKS_1K   1000
#define NUM_OF_BLOCKS_5K   5000
#define NUM_OF_BLOCKS_10K  10000
#define NUM_OF_BLOCKS_100K 100000
#define NUM_OF_BLOCKS_1M   1000000


void CreateMalloc();

void Debug_InitMalloc();
void Debug_Malloc_Block();
void Debug_Free_Block();
void Debug_Realloc_Block();

void Debug_Aligned_Malloc_Block();
void Debug_Aligned_Realloc_Block();

void Test_Malloc_Blocks1();
void Test_Malloc_Blocks2();

void Test_Malloc_10BlocksPow2();
void Test_Malloc_100BlocksPow2();
void Test_Malloc_5KBlocksPow2();
void Test_Malloc_10KBlocksPow2();
void Test_Malloc_And_Free_Aligned_Blocks1();

void Test_Malloc_PoolOverflow();

