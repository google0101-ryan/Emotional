// (c) Copyright 2022 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

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
	used_registers[RSP] = true;
}

int RegisterAllocator::AllocHostRegister()
{
	for (int i = 0; i < 8; i++)
	{
		if (!used_registers[i])
		{
			used_registers[i] = true;
			if (i < 4)
			{
				int ind = i * 2;
				used_registers_8[ind] = true;
				used_registers_8[ind+1] = true;
			}
			return i;
		}
	}

	printf("[JIT/RegAlloc]: Ran out of host registers!\n");
	exit(1);
}

void RegisterAllocator::MarkRegUsed(int reg)
{
	used_registers[reg] = true;
	if (reg < 4)
	{
		int ind = reg * 2;
		used_registers_8[ind] = true;
		used_registers_8[ind+1] = true;
	}
}
