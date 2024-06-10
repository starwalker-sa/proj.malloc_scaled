#include "test_vm_block.h"

int main()
{
	Test_Reserve_And_Release_Area();
	Test_Malloc_And_Free_Block();
	Test_Malloc_Merge_Blocks();
	Test_Area_Overflow();

	return 0;
}