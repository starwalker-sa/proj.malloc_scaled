#include "test_malloc.h"

//TMallocScaled* MemoryAllocator = nullptr;


/*
-------------------

	TEST MALLOC

-------------------
*/

void Debug_Malloc_Block()
{
	void* Ptr[8] = {nullptr};
	for (int i = 0; i < 8; ++i)
	{
		Ptr[i] = Malloc(2 * 1024 * 1024);
	}

	for (int i = 0; i < 8; ++i)
	{
		Free(Ptr[i]);
	}
}

void Debug_Aligned_Malloc_Block()
{
	void* Ptr0 = Malloc(32, 256);
	void* Ptr1 = Malloc(32, 0);
}

void Debug_Free_Block()
{
	void* Ptr0 = Malloc(32);
	Free(Ptr0);
}

void Debug_Realloc_Block()
{

	void* Ptr = Malloc(32);
	Ptr = Realloc(Ptr, 40);
	Ptr = Realloc(Ptr, 128);
	Ptr = Realloc(Ptr, 64);
	Ptr = Realloc(Ptr, 700);

}


void Debug_Aligned_Realloc_Block()
{

	void* Ptr = Malloc(32);
	Ptr = Realloc(Ptr, 32, 1024);
}

void Test_Malloc_Blocks1()
{
	void* Ptr[32] = {nullptr};

	Ptr[0] = Malloc(BLOCK_SIZE_16B);
	Ptr[1] = Malloc(BLOCK_SIZE_32B);
	Ptr[2] = Malloc(BLOCK_SIZE_64B);
	Ptr[3] = Malloc(BLOCK_SIZE_128B);
	Ptr[4] = Malloc(BLOCK_SIZE_256B);
	Ptr[5] = Malloc(BLOCK_SIZE_512B);
	Ptr[6] = Malloc(BLOCK_SIZE_1KB);
	Ptr[7] = Malloc(BLOCK_SIZE_2KB);
	Ptr[8] = Malloc(BLOCK_SIZE_4KB);
	Ptr[9] = Malloc(BLOCK_SIZE_8KB);
	Ptr[10] = Malloc(BLOCK_SIZE_16KB);
	Ptr[11] = Malloc(BLOCK_SIZE_32KB);
	Ptr[12] = Malloc(BLOCK_SIZE_64KB);
	Ptr[13] = Malloc(BLOCK_SIZE_128KB);
	Ptr[14] = Malloc(BLOCK_SIZE_256KB);
	Ptr[15] = Malloc(BLOCK_SIZE_512KB);
	Ptr[16] = Malloc(BLOCK_SIZE_1MB);
	Ptr[17] = Malloc(BLOCK_SIZE_2MB);
	Ptr[18] = Malloc(BLOCK_SIZE_4MB);
	Ptr[19] = Malloc(BLOCK_SIZE_8MB);
	Ptr[20] = Malloc(BLOCK_SIZE_16MB);
	Ptr[21] = Malloc(BLOCK_SIZE_32MB);
	Ptr[22] = Malloc(BLOCK_SIZE_64MB);
	Ptr[23] = Malloc(BLOCK_SIZE_128MB);
	Ptr[24] = Malloc(BLOCK_SIZE_256MB);
	Ptr[25] = Malloc(BLOCK_SIZE_512MB);
	Ptr[26] = Malloc(BLOCK_SIZE_1GB);
	Ptr[27] = Malloc(BLOCK_SIZE_2GB);
	Ptr[28] = Malloc(BLOCK_SIZE_3GB);
	Ptr[29] = Malloc(BLOCK_SIZE_4GB);
	Ptr[30] = Malloc(BLOCK_SIZE_5GB);
	Ptr[31] = Malloc(BLOCK_SIZE_8GB);

}


void Test_Malloc_And_Free_Aligned_Blocks1()
{
	void* Ptr[32] = { nullptr };

	Ptr[0] = Malloc(BLOCK_SIZE_16B, 128);
	Ptr[1] = Malloc(BLOCK_SIZE_32B, 256);
	Ptr[2] = Malloc(BLOCK_SIZE_64B, 512);
	Ptr[3] = Malloc(BLOCK_SIZE_128B, 1024);
	Ptr[4] = Malloc(BLOCK_SIZE_256B, 4096);
	Ptr[5] = Malloc(BLOCK_SIZE_512B, 4096);
	Ptr[6] = Malloc(BLOCK_SIZE_2MB, 4096);
	Ptr[7] = Malloc(BLOCK_SIZE_4MB, 1024);

	Free(Ptr[0]);
	Free(Ptr[1]);
	Free(Ptr[2]);
	Free(Ptr[3]);
	Free(Ptr[4]);
	Free(Ptr[5]);
	Free(Ptr[6]);
	Free(Ptr[7]);

}

void Test_Malloc_Blocks2()
{
	void* Ptr[32] = { nullptr };

	Ptr[0] = Malloc(BLOCK_SIZE_4B);
	Ptr[1] = Malloc(BLOCK_SIZE_19B);
	Ptr[2] = Malloc(BLOCK_SIZE_33B);
	Ptr[3] = Malloc(BLOCK_SIZE_77B);
	Ptr[4] = Malloc(BLOCK_SIZE_111B);
	Ptr[5] = Malloc(BLOCK_SIZE_444B);
	Ptr[6] = Malloc(BLOCK_SIZE_700B);
	Ptr[7] = Malloc(BLOCK_SIZE_1500B);
	Ptr[8] = Malloc(BLOCK_SIZE_2500B);
	Ptr[9] = Malloc(BLOCK_SIZE_5000B);
	Ptr[10] = Malloc(BLOCK_SIZE_12000B);
	Ptr[11] = Malloc(BLOCK_SIZE_20000B);
	Ptr[12] = Malloc(BLOCK_SIZE_40000B);
	Ptr[13] = Malloc(BLOCK_SIZE_110000B);
	Ptr[14] = Malloc(BLOCK_SIZE_200000B);
	Ptr[15] = Malloc(BLOCK_SIZE_700000B);
	Ptr[16] = Malloc(BLOCK_SIZE_1500000B);
	Ptr[17] = Malloc(BLOCK_SIZE_3000000B);
	Ptr[18] = Malloc(BLOCK_SIZE_7000000B);
	Ptr[19] = Malloc(BLOCK_SIZE_12000000B);
	Ptr[20] = Malloc(BLOCK_SIZE_20000000B);
	Ptr[21] = Malloc(BLOCK_SIZE_40000000B);
	Ptr[22] = Malloc(BLOCK_SIZE_80000000B);
	Ptr[23] = Malloc(BLOCK_SIZE_180000000B);
	Ptr[24] = Malloc(BLOCK_SIZE_300000000B);
	Ptr[25] = Malloc(BLOCK_SIZE_300000000B);
	Ptr[26] = Malloc(BLOCK_SIZE_300000000B);
	Ptr[27] = Malloc(BLOCK_SIZE_300000000B);
	Ptr[28] = Malloc(BLOCK_SIZE_300000000B);
	Ptr[29] = Malloc(BLOCK_SIZE_300000000B);
	Ptr[30] = Malloc(BLOCK_SIZE_300000000B);
}

void Test_Free_Blocks1()
{
	void* Ptr[32] = { nullptr };

	Ptr[0] = Malloc(BLOCK_SIZE_16B);
	Ptr[1] = Malloc(BLOCK_SIZE_32B);
	Ptr[2] = Malloc(BLOCK_SIZE_64B);
	Ptr[3] = Malloc(BLOCK_SIZE_128B);
	Ptr[4] = Malloc(BLOCK_SIZE_256B);
	Ptr[5] = Malloc(BLOCK_SIZE_512B);
	Ptr[6] = Malloc(BLOCK_SIZE_1KB);
	Ptr[7] = Malloc(BLOCK_SIZE_2KB);
	Ptr[8] = Malloc(BLOCK_SIZE_4KB);
	Ptr[9] = Malloc(BLOCK_SIZE_8KB);
	Ptr[10] = Malloc(BLOCK_SIZE_16KB);
	Ptr[11] = Malloc(BLOCK_SIZE_32KB);
	Ptr[12] = Malloc(BLOCK_SIZE_64KB);
	Ptr[13] = Malloc(BLOCK_SIZE_128KB);
	Ptr[14] = Malloc(BLOCK_SIZE_256KB);
	Ptr[15] = Malloc(BLOCK_SIZE_512KB);
	Ptr[16] = Malloc(BLOCK_SIZE_1MB);
	Ptr[17] = Malloc(BLOCK_SIZE_2MB);
	Ptr[18] = Malloc(BLOCK_SIZE_4MB);
	Ptr[19] = Malloc(BLOCK_SIZE_8MB);
	Ptr[20] = Malloc(BLOCK_SIZE_16MB);
	Ptr[21] = Malloc(BLOCK_SIZE_32MB);
	Ptr[22] = Malloc(BLOCK_SIZE_64MB);
	Ptr[23] = Malloc(BLOCK_SIZE_128MB);
	Ptr[24] = Malloc(BLOCK_SIZE_256MB);
	Ptr[25] = Malloc(BLOCK_SIZE_512MB);
	Ptr[26] = Malloc(BLOCK_SIZE_1GB);
	Ptr[27] = Malloc(BLOCK_SIZE_2GB);
	Ptr[28] = Malloc(BLOCK_SIZE_3GB);
	Ptr[29] = Malloc(BLOCK_SIZE_4GB);
	Ptr[30] = Malloc(BLOCK_SIZE_5GB);
	Ptr[31] = Malloc(BLOCK_SIZE_8GB);


	Free(Ptr[0]);
	Free(Ptr[1]);
	Free(Ptr[2]);
	Free(Ptr[3]);
	Free(Ptr[4]);
	Free(Ptr[5]);
	Free(Ptr[6]);
	Free(Ptr[7]);
	Free(Ptr[8]);
	Free(Ptr[9]);
	Free(Ptr[10]);
	Free(Ptr[11]);
	Free(Ptr[12]);
	Free(Ptr[13]);
	Free(Ptr[14]);
	Free(Ptr[15]);
	Free(Ptr[16]);
	Free(Ptr[17]);
	Free(Ptr[18]);
	Free(Ptr[19]);
	Free(Ptr[20]);
	Free(Ptr[21]);
	Free(Ptr[22]);
	Free(Ptr[23]);
	Free(Ptr[24]);
	Free(Ptr[25]);
	Free(Ptr[26]);
	Free(Ptr[27]);
	Free(Ptr[28]);
	Free(Ptr[29]);
	Free(Ptr[30]);
	Free(Ptr[31]);
}

void Test_Realloc_Blocks1()
{
	void* Ptr[32] = { nullptr };

	Ptr[0] = Malloc(BLOCK_SIZE_16B);
	Ptr[1] = Malloc(BLOCK_SIZE_32B);
	Ptr[2] = Malloc(BLOCK_SIZE_64B);
	Ptr[3] = Malloc(BLOCK_SIZE_128B);
	Ptr[4] = Malloc(BLOCK_SIZE_256B);
	Ptr[5] = Malloc(BLOCK_SIZE_512B);
	Ptr[6] = Malloc(BLOCK_SIZE_1KB);
	Ptr[7] = Malloc(BLOCK_SIZE_2KB);
	Ptr[8] = Malloc(BLOCK_SIZE_4KB);
	Ptr[9] = Malloc(BLOCK_SIZE_8KB);
	Ptr[10] = Malloc(BLOCK_SIZE_16KB);
	Ptr[11] = Malloc(BLOCK_SIZE_32KB);
	Ptr[12] = Malloc(BLOCK_SIZE_64KB);
	Ptr[13] = Malloc(BLOCK_SIZE_128KB);
	Ptr[14] = Malloc(BLOCK_SIZE_256KB);
	Ptr[15] = Malloc(BLOCK_SIZE_512KB);
	Ptr[16] = Malloc(BLOCK_SIZE_1MB);
	Ptr[17] = Malloc(BLOCK_SIZE_2MB);
	Ptr[18] = Malloc(BLOCK_SIZE_4MB);
	Ptr[19] = Malloc(BLOCK_SIZE_8MB);
	Ptr[20] = Malloc(BLOCK_SIZE_16MB);
	Ptr[21] = Malloc(BLOCK_SIZE_32MB);
	Ptr[22] = Malloc(BLOCK_SIZE_64MB);
	Ptr[23] = Malloc(BLOCK_SIZE_128MB);
	Ptr[24] = Malloc(BLOCK_SIZE_256MB);
	Ptr[25] = Malloc(BLOCK_SIZE_512MB);
	Ptr[26] = Malloc(BLOCK_SIZE_1GB);
	Ptr[27] = Malloc(BLOCK_SIZE_2GB);
	Ptr[28] = Malloc(BLOCK_SIZE_3GB);
	Ptr[29] = Malloc(BLOCK_SIZE_4GB);
	Ptr[30] = Malloc(BLOCK_SIZE_5GB);
	Ptr[31] = Malloc(BLOCK_SIZE_8GB);


	Ptr[0] = Realloc(Ptr[0], BLOCK_SIZE_16B * 2);
	Ptr[1] = Realloc(Ptr[1], BLOCK_SIZE_32B / 2);
	Ptr[2] = Realloc(Ptr[2], BLOCK_SIZE_64B * 3);
	Ptr[3] = Realloc(Ptr[3], BLOCK_SIZE_128B / 2);
	Ptr[4] = Realloc(Ptr[4], BLOCK_SIZE_256B + 20);
	Ptr[5] = Realloc(Ptr[5], BLOCK_SIZE_512B + 20);
	Ptr[6] = Realloc(Ptr[6], BLOCK_SIZE_1KB  + 100);
	Ptr[7] = Realloc(Ptr[7], BLOCK_SIZE_2KB / 2);
	Ptr[8] = Realloc(Ptr[8], BLOCK_SIZE_4KB * 2);
	Ptr[9] = Realloc(Ptr[9], BLOCK_SIZE_8KB * 3);
	Ptr[10] = Realloc(Ptr[10], BLOCK_SIZE_16KB / 5);
	Ptr[11] = Realloc(Ptr[11], BLOCK_SIZE_32KB / 3);
	Ptr[12] = Realloc(Ptr[12], BLOCK_SIZE_64KB * 3);
	Ptr[13] = Realloc(Ptr[13], BLOCK_SIZE_128KB + 1000);
	Ptr[14] = Realloc(Ptr[14], BLOCK_SIZE_256KB + 3000);
	Ptr[15] = Realloc(Ptr[15], BLOCK_SIZE_512KB + 30);
	Ptr[16] = Realloc(Ptr[16], BLOCK_SIZE_1MB + 60000);
	Ptr[17] = Realloc(Ptr[17], BLOCK_SIZE_2MB + 77777);
	Ptr[18] = Realloc(Ptr[18], BLOCK_SIZE_4MB * 1.3f);
	Ptr[19] = Realloc(Ptr[19], BLOCK_SIZE_8MB / 1.4f);
	Ptr[20] = Realloc(Ptr[20], BLOCK_SIZE_16MB + 233234);
	Ptr[21] = Realloc(Ptr[21], BLOCK_SIZE_32MB / 5);
	Ptr[22] = Realloc(Ptr[22], BLOCK_SIZE_64MB + 8000000);
	Ptr[23] = Realloc(Ptr[23], BLOCK_SIZE_128MB + 10000000);
	Ptr[24] = Realloc(Ptr[24], BLOCK_SIZE_256MB - 50000000);
	Ptr[25] = Realloc(Ptr[25], BLOCK_SIZE_512MB / 2);
	Ptr[26] = Realloc(Ptr[26], BLOCK_SIZE_1GB * 2);
	Ptr[27] = Realloc(Ptr[27], BLOCK_SIZE_2GB / 1.5f);
	Ptr[28] = Realloc(Ptr[28], BLOCK_SIZE_3GB / 3);
	Ptr[29] = Realloc(Ptr[29], BLOCK_SIZE_4GB / 3);
	Ptr[30] = Realloc(Ptr[30], BLOCK_SIZE_5GB * 2);
	Ptr[31] = Realloc(Ptr[31], BLOCK_SIZE_8GB / 2);

}


void Test_Malloc_10BlocksPow2()
{
	uint32 Cnt = NUM_OF_BLOCKS_10;
	++Cnt;
	while (Cnt)
	{
		void* Ptr = Malloc(BLOCK_SIZE_512B);
		--Cnt;
	}
}

void Test_Malloc_100BlocksPow2()
{
	uint32 Cnt = NUM_OF_BLOCKS_100;
	++Cnt;
	while (Cnt)
	{
		void* Ptr = Malloc(BLOCK_SIZE_512B);
		--Cnt;
	}
}


void Test_Malloc_5KBlocksPow2()
{
	uint32 Cnt = NUM_OF_BLOCKS_5K;
	++Cnt;
	while (Cnt)
	{
		void* Ptr = Malloc(BLOCK_SIZE_512B);
		--Cnt;
	}
}

void Test_Malloc_10KBlocksPow2()
{
	uint32 Cnt = NUM_OF_BLOCKS_10K;
	++Cnt;
	while (Cnt)
	{
		void* Ptr = Malloc(BLOCK_SIZE_512B);
		--Cnt;
	}
}

void Test_Malloc_PoolOverflow()
{
	uint32 Cnt = NUM_OF_BLOCKS_10K;
	Cnt -= 2;
	while (Cnt)
	{
		void* Ptr = Malloc(BLOCK_SIZE_512B);
		
		--Cnt;
	}

	void* Ptr1 = Malloc(BLOCK_SIZE_512B);
	void* Ptr2 = Malloc(BLOCK_SIZE_512B);
	void* Ptr3 = Malloc(BLOCK_SIZE_512B);

}

void Test_Free_NBlocks512()
{
	uint32 Cnt = NUM_OF_BLOCKS_10;

	void* Ptrs[NUM_OF_BLOCKS_10];

	while (Cnt)
	{
		Ptrs[Cnt - 1] = Malloc(BLOCK_SIZE_512B);
		--Cnt;
	}

	Cnt = NUM_OF_BLOCKS_10;
	while (Cnt)
	{
		Free(Ptrs[Cnt - 1]);
		--Cnt;
	}
}



