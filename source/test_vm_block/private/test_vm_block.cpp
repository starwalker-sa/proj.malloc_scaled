#include "vm_block.h"
#include "test_vm_block.h"

/*
-------------------

	TEST VM BLOCK

-------------------
*/

void Test_Reserve_And_Release_Area()
{
	bool Ok = false;
	Ok = TVMBlock::Init();
	TVMBlock::Release();

	IPageMalloc* PageMalloc = TPageMalloc::GetPageMalloc();

	TMemoryBlock MemBlock[8];

	Ok = PageMalloc->Reserve(MemBlock[0]);
	Ok = PageMalloc->Reserve(MemBlock[1]);
	Ok = PageMalloc->Reserve(BLOCK_SIZE_64KB,  MemBlock[2]);
	Ok = PageMalloc->Reserve(BLOCK_SIZE_128MB, MemBlock[3]);
	Ok = PageMalloc->Reserve(BLOCK_SIZE_256MB, MemBlock[4]);
	Ok = PageMalloc->Reserve(BLOCK_SIZE_1GB,   MemBlock[5]);
	Ok = PageMalloc->Reserve(BLOCK_SIZE_2GB,   MemBlock[6]);
	Ok = PageMalloc->Reserve(BLOCK_SIZE_8GB,   MemBlock[7]);
	Ok = PageMalloc->Release();
}

void Test_Malloc_And_Free_Block()
{
	bool Ok = false;
	Ok = TVMBlock::Init();

	TVMBlock VMBlocks[16];

	Ok = VMBlocks[0].Allocate(BLOCK_SIZE_64KB);
	VMBlocks[0].Free();


	Ok = VMBlocks[0].Allocate(BLOCK_SIZE_16KB);
	Ok = VMBlocks[1].Allocate(BLOCK_SIZE_16KB); // attempt to twice alloc;

	Ok = VMBlocks[2].Allocate(BLOCK_SIZE_64KB);
	Ok = VMBlocks[3].Allocate(BLOCK_SIZE_128KB);

	VMBlocks[0].Free();
	VMBlocks[1].Free();

	VMBlocks[2].Free();
	VMBlocks[3].Free();

	TVMBlock::Release();
}

void Test_Malloc_Merge_Blocks()
{
	bool Ok = false;
	Ok = TVMBlock::Init();

	TVMBlock VMBlocks[6];

	// at the begining of an area
	Ok = VMBlocks[0].Allocate(BLOCK_SIZE_128KB);
	Ok = VMBlocks[1].Allocate(BLOCK_SIZE_128KB);
	Ok = VMBlocks[2].Allocate(BLOCK_SIZE_128KB);

	VMBlocks[1].Free();
	VMBlocks[0].Free();
	VMBlocks[2].Free();

	// at the middle of an area
	Ok = VMBlocks[0].Allocate(BLOCK_SIZE_128KB);
	Ok = VMBlocks[1].Allocate(BLOCK_SIZE_128KB);
	Ok = VMBlocks[2].Allocate(BLOCK_SIZE_128KB);
	Ok = VMBlocks[3].Allocate(BLOCK_SIZE_128KB);
	Ok = VMBlocks[4].Allocate(BLOCK_SIZE_128KB);

	VMBlocks[2].Free();
	VMBlocks[1].Free();
	VMBlocks[3].Free();

	TVMBlock::Release();

	// at the end of an area
	Ok = VMBlocks[0].Allocate(BLOCK_SIZE_32MB);
	Ok = VMBlocks[1].Allocate(BLOCK_SIZE_32MB);
	Ok = VMBlocks[2].Allocate(BLOCK_SIZE_32MB);
	Ok = VMBlocks[3].Allocate(BLOCK_SIZE_32MB);

	VMBlocks[3].Free();
	VMBlocks[1].Free();
	VMBlocks[2].Free();

	TVMBlock::Release();
}

void Test_Area_Overflow()
{
	bool Ok = false;
	Ok = TVMBlock::Init();

	TVMBlock VMBlocks[9];

	Ok = VMBlocks[0].Allocate(BLOCK_SIZE_16MB);
	Ok = VMBlocks[1].Allocate(BLOCK_SIZE_32MB);
	Ok = VMBlocks[2].Allocate(BLOCK_SIZE_64MB);
	Ok = VMBlocks[3].Allocate(BLOCK_SIZE_64MB);
	Ok = VMBlocks[4].Allocate(BLOCK_SIZE_256MB);
	Ok = VMBlocks[5].Allocate(BLOCK_SIZE_512MB);
	Ok = VMBlocks[6].Allocate(BLOCK_SIZE_1GB);
	Ok = VMBlocks[7].Allocate(BLOCK_SIZE_4GB);
	Ok = VMBlocks[8].Allocate(BLOCK_SIZE_8GB);

	VMBlocks[0].Free();
	VMBlocks[1].Free();
	VMBlocks[2].Free();
	VMBlocks[3].Free();
	VMBlocks[4].Free();
	VMBlocks[5].Free();
	VMBlocks[6].Free();
	VMBlocks[7].Free();
	VMBlocks[8].Free();

	TVMBlock::Release();
}

