#include "reg_alloc.h"
#include <cstdio>
#include <cstdlib>

RegisterAllocator::RegisterAllocator()
{
	Reset();
	// Here just in case I have to do any initialization in the future
}

void RegisterAllocator::Reset()
{
	used_registers.fill(false);
	used_registers_8.fill(false);
	used_registers[RBP] = true;
}

int RegisterAllocator::AllocHostRegister()
{
	for (int i = 0; i < 8; i++)
	{
		if (!used_registers[i])
		{
			used_registers[i] = true;
			return i;
		}
	}

	printf("[JIT/RegAlloc]: Ran out of host registers!\n");
	exit(1);
}