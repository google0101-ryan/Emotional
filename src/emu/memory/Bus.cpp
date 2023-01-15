// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include "Bus.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>

uint8_t BiosRom[0x400000];
uint8_t spr[0x4000];
uint8_t ram[0x2000000];

uint32_t MCH_DRD, MCH_RICM;
uint32_t rdram_sdevid;

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

std::ofstream console;

void Bus::LoadBios(uint8_t *data)
{
	memcpy(BiosRom, data, 0x400000);
	console.open("console.txt");
}

void Bus::Dump()
{
	std::ofstream dump("spr.dump");

	for (int i = 0; i < 0x4000; i++)
	{
		dump << (char)spr[i];
	}

	dump.close();

	dump.open("ram.dump");

	for (int i = 0; i < 0x2000000; i++)
	{
		dump << (char)ram[i];
	}

	dump.close();
}

uint64_t Bus::Read64(uint32_t addr)
{
	addr = Translate(addr);

	if (addr >= 0x1fc00000 && addr < 0x20000000)
		return *(uint64_t*)&BiosRom[addr - 0x1fc00000];
	if (addr >= 0x70000000 && addr < 0x70004000)
		return *(uint64_t*)&spr[addr - 0x70000000];
	
	printf("Read64 from unknown address 0x%08x\n", addr);
	exit(1);
}

uint32_t Bus::Read32(uint32_t addr)
{
	addr = Translate(addr);

	if (addr >= 0x1fc00000 && addr < 0x20000000)
		return *(uint32_t*)&BiosRom[addr - 0x1fc00000];
	if (addr >= 0x70000000 && addr < 0x70004000)
		return *(uint32_t*)&spr[addr - 0x70000000];

	switch (addr)
	{
	case 0x1000f130:
	case 0x1000f430:
		return 0;
	case 0x1000f440:
	{
		uint8_t SOP = (MCH_RICM >> 6) & 0xF;
		uint8_t SA = (MCH_RICM >> 16) & 0xFFF;
		if (!SOP)
		{
			switch (SA)
			{
			case 0x21:
				if (rdram_sdevid < 2)
				{
					rdram_sdevid++;
					return 0x1F;
				}
				return 0;
			case 0x23:
				return 0x0D0D;
			case 0x24:
				return 0x0090;
			case 0x40:
				return MCH_RICM & 0x1F;
			}
		}
		return 0;
	}
	}
	
	printf("Read32 from unknown address 0x%08x\n", addr);
	exit(1);
}

uint16_t Bus::Read16(uint32_t addr)
{
	addr = Translate(addr);
	
	if (addr >= 0x1fc00000 && addr < 0x20000000)
		return *(uint16_t*)&BiosRom[addr - 0x1fc00000];
	if (addr >= 0x70000000 && addr < 0x70004000)
		return *(uint16_t*)&spr[addr - 0x70000000];

	printf("Read16 from unknown address 0x%08x\n", addr);
	exit(1);
}

uint8_t Bus::Read8(uint32_t addr)
{
	addr = Translate(addr);


	if (addr >= 0x1fc00000 && addr < 0x20000000)
		return BiosRom[addr - 0x1fc00000];
	if (addr >= 0x70000000 && addr < 0x70004000)
		return spr[addr - 0x70000000];
	
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
	
	if (addr >= 0x70000000 && addr < 0x70004000)
	{
		*(uint32_t*)&spr[addr - 0x70000000] = data;
		return;
	}

	switch (addr)
	{
	case 0x1000f100: // Some weird RDRAM stuff
	case 0x1000f120:
	case 0x1000f140:
	case 0x1000f150:
	case 0x1000f410:
	case 0x1000f480:
	case 0x1000f490:
		return;
	case 0x1000f500: // EE TLB enable?
		return;
	case 0x1000f430:
	{
		uint8_t SA = (data >> 16) & 0xFFF;
		uint8_t SBC = (data >> 6) & 0xF;

		if (SA == 0x21 && SBC == 0x1 && ((MCH_DRD >> 7) & 1) == 0)
			rdram_sdevid = 0;
		
		MCH_RICM = data & ~0x80000000;
		return;
	}
	case 0x1000f440:
		MCH_DRD = data;
		return;
	}

	printf("Write32 to unknown address 0x%08x\n", addr);
	exit(1);
}

void Bus::Write16(uint32_t addr, uint16_t data)
{
	addr = Translate(addr);
	
	if (addr >= 0x70000000 && addr < 0x70004000)
	{
		*(uint16_t*)&spr[addr - 0x70000000] = data;
		return;
	}

	printf("Write16 to unknown address 0x%08x\n", addr);
	exit(1);
}

void Bus::Write8(uint32_t addr, uint8_t data)
{
	addr = Translate(addr);

	if (addr >= 0x70000000 && addr < 0x70004000)
	{
		spr[addr - 0x70000000] = data;
		return;
	}

	switch (addr)
	{
	case 0x1000f180:
		console << (char)data;
		console.flush();
		return;
	}

	printf("Write8 to unknown address 0x%08x\n", addr);
	exit(1);
}
