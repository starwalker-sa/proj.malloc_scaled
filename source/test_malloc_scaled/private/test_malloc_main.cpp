#include "test_malloc.h"
#include "timer.h"

int main()
{
	Debug_Malloc_Block();
	Debug_Free_Block();
	Debug_Realloc_Block();

	Debug_Aligned_Malloc_Block();
	Debug_Aligned_Realloc_Block();

	Test_Malloc_Blocks1();
	Test_Malloc_Blocks2();
	Test_Malloc_And_Free_Aligned_Blocks1();

	return 0;
}