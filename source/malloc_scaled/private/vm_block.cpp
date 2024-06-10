
#include "vm_block.h"

IPageMalloc* TVMBlock::PageMalloc   = nullptr;

bool TVMBlock::Init()
{
	if (!PageMalloc)
	{
		PageMalloc = TPageMalloc::GetPageMalloc();
		if (!PageMalloc)
		{
			return false;
		}
	}

	return true;
}

void TVMBlock::Release()
{
	if (PageMalloc)
	{
		PageMalloc->Release();
	}
}

bool TVMBlock::Allocate(TSize Size)
{
	bool Ok = false;

	if (PageMalloc)
	{
		Ok = PageMalloc->AllocateBlock(Size, VMBlock);
		
		if (!Ok)
		{
			TMemoryBlock Block;
			Ok = PageMalloc->Reserve(Size, Block);

			if (Ok)
			{
				Ok = PageMalloc->AllocateBlock(Size, VMBlock);
			}
		}

		if (Ok)
		{
			Allocated = true;
			return true;
		}
	}

	return false;
}

bool TVMBlock::Allocate(void* Address, TSize Size)
{
	return false;
}

void TVMBlock::Free()
{
	if (PageMalloc)
	{
		void* Base = VMBlock.GetBase();
		TSize Size = VMBlock.GetSize();
		bool Ok = PageMalloc->FreeBlock(TMemoryBlock{ Base, Size });

		if (!Ok)
		{
			// log: POSSIBLE LACK OF MEMORY;
			printf("PAGE MALLOC: CANNOT RELEASE BLOCK: Address: %p, Size: %llu; MIGHT BE LACK OF MEMORY\n",
				Base, Size);
		}
	}

	Allocated = false;
}

bool TVMBlock::SetProtection(void* Offset, TSize Size, TMemoryBlockAccess AccessFlag)
{
	if (PageMalloc)
	{
		return PageMalloc->SetProtection(TMemoryBlock{ Offset, Size }, AccessFlag);
	}

	return false;
}

bool TVMBlock::IsPagingSupported()
{
	return (bool)PageMalloc->GetPageSize();
}

bool TVMBlock::IsProtectionSupported()
{
	return PageMalloc->IsProtectionSupported();
}

TSize TVMBlock::GetPageSize()
{
	return PageMalloc->GetPageSize();
}

bool TVMBlock::IsAllocated()
{
	return Allocated;
}

TSize TVMBlock::GetAllocatedSize()
{
	return VMBlock.GetSize();
}

void* TVMBlock::GetBase()
{
	return VMBlock.GetBase();
}

void* TVMBlock::GetEnd()
{
	return VMBlock.GetEnd();
}