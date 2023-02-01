// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include <emu/gpu/gs.h>

#include <cstdio>
#include <cstdlib>

namespace GS
{

union GS_CSR
{
	uint32_t data;
	struct
	{
		uint32_t signal : 1;
		uint32_t finish : 1;
		uint32_t hsint : 1;
		uint32_t vsint : 1;
		uint32_t edwint : 1;
		uint32_t : 3;
		uint32_t flush : 1;
		uint32_t reset : 1;
		uint32_t : 2;
		uint32_t nfield : 1;
		uint32_t field : 1;
		uint32_t fifo : 2;
		uint32_t rev : 8;
		uint32_t id : 8;
	};
} csr;

void WriteGSCSR(uint64_t data)
{
	csr.signal &= ~(data & 1);
	if (data & 2)
	{
		csr.finish = false;
	}
	if (data & 4)
	{
		csr.hsint = false;
	}
	if (data & 8)
	{
		csr.vsint = false;
	}
	if (data & 0x200)
	{
		csr.data = 0;
		csr.fifo = 0x1;
		printf("[emu/GS]: Reset\n");
	}
}

}  // namespace GS
