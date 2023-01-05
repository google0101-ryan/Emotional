// (c) Copyright 2022 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include "Bus.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

uint8_t BiosRom[0x400000];
uint8_t spr[0x4000];

static uint32_t Translate(uint32_t addr)
{
    constexpr uint32_t KUSEG_MASKS[8] = 
    {
        /* KUSEG: Don't touch the address, it's fine */
        0xffffffff, 0xfffffff, 0xfffffff, 0xffffffff,
        /* KSEG0: Strip the MSB (0x8 -> 0x0 and 0x9 -> 0x1) */
        0x7fffffff,
        /* KSEG1: Strip the 3 MSB's (0xA -> 0x0 and 0xB -> 0x1) */
        0x1fffffff,
        /* KSEG2: Don't touch the address, it's fine */
        0xffffffff, 0x1fffffff,
    };

	if (addr >= 0xFFFF8000) [[unlikely]]
	{
		return (addr - 0xFFFF8000) + 0x78000;
	}

    addr &= KUSEG_MASKS[addr >> 29];

	return addr;
}

void Bus::LoadBios(uint8_t *data)
{
	memcpy(BiosRom, data, 0x400000);
}

uint32_t Bus::Read32(uint32_t addr)
{
	addr = Translate(addr);

	if (addr >= 0x1fc00000 && addr < 0x20000000)
		return *(uint32_t*)&BiosRom[addr - 0x1fc00000];
	
	printf("Read from unknown address 0x%08x\n", addr);
	exit(1);
}

void Bus::Write64(uint32_t addr, uint64_t data)
{
	addr = Translate(addr);

	if (addr >= 0x70000000 && addr < 0x70004000)
	{
		*(uint64_t*)&spr[addr - 0x70000000] = data;
		return;
	}

	printf("Write64 to unknown address 0x%08x\n", addr);
	exit(1);
}

void Bus::Write32(uint32_t addr, uint32_t data)
{
	addr = Translate(addr);

	switch (addr)
	{
	case 0x1000f500: // EE TLB enable?
		return;
	}

	printf("Write to unknown address 0x%08x\n", addr);
	exit(1);
}
