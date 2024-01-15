// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include <emu/memory/Bus.h>

#include <emu/cpu/ee/vu.h>
#include <emu/cpu/ee/vif.h>
#include <emu/dev/sif.h>
#include <emu/gpu/gs.h>
#include <emu/cpu/ee/EmotionEngine.h>

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>

#include <emu/gpu/gif.hpp>
#include <emu/cpu/ee/dmac.hpp>
#include "Bus.h"

uint8_t* BiosRom;
uint8_t spr[0x4000];
uint8_t ram[0x2000000];
uint8_t* iop_ram;

uint32_t MCH_DRD, MCH_RICM;
uint32_t rdram_sdevid;

uint32_t INTC_MASK = 0, INTC_STAT = 0;
uint32_t Bus::I_MASK = 0, Bus::I_STAT = 0, Bus::I_CTRL = 0;
uint32_t Bus::spu2_stat = 0;

uint32_t Translate(uint32_t addr)
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

	if ((addr & 0xF0000000) != 0x70000000)
	    addr &= 0x1FFFFFFF;

	return addr;
}

std::ofstream console;

void Bus::LoadBios(uint8_t *data)
{
	BiosRom = new uint8_t[0x400000];
	iop_ram = new uint8_t[0x200000];

	memcpy(BiosRom, data, 0x400000);
	console.open("console.txt");

	GS::Initialize();
	VectorUnit::VU0::Init();
}

void Bus::Dump()
{
	std::ofstream dump("spr.dump");

	for (int i = 0; i < 0x4000; i++)
	{
		dump << static_cast<char>(spr[i]);
	}

	dump.close();

	dump.open("ram.dump");

	for (int i = 0; i < 0x2000000; i++)
	{
		dump << static_cast<char>(ram[i]);
	}

	dump.close();

	dump.open("psx_cpu_ram.dump");

	for (int i = 0; i < 0x200000; i++)
	{
		dump << static_cast<char>(iop_ram[i]);
	}

	dump.close();

	printf("[emu/Bus]: IOP_ISTAT: 0x%08x, IOP_IMASK: 0x%08x\n", I_STAT, I_MASK);
}

uint8_t *Bus::GetRamPtr()
{
    return ram;
}

uint8_t *Bus::GetSprPtr()
{
    return spr;
}

uint8_t *Bus::GetPtrForAddress(uint32_t addr)
{
	addr = Translate(addr);

	if (addr < 0x10000000)
	{
		addr &= (1024*1024*32)-1;
		return ram+addr;
	}
	if (addr >= 0x1FC00000 && addr < 0x20000000)
	{
		addr &= (1024*1024*4)-1;
		return BiosRom+addr;
	}

	return nullptr;
}

uint128_t Bus::Read128(uint32_t addr)
{
	addr = Translate(addr);

	if (addr < 0x2000000)
		return {*reinterpret_cast<__uint128_t*>(&ram[addr])};
	if (addr >= 0x1C000000 && addr < 0x1C200000)
		return {*reinterpret_cast<__uint128_t*>(&iop_ram[addr-0x1C000000])};

	printf("Read128 from unknown address 0x%08x\n", addr);
	exit(1);
}

uint64_t Bus::Read64(uint32_t addr)
{
	addr = Translate(addr);

	if (addr >= 0x1fc00000 && addr < 0x20000000)
		return *reinterpret_cast<uint64_t*>(&BiosRom[addr - 0x1fc00000]);
	if (addr >= 0x70000000 && addr < 0x70004000)
		return *reinterpret_cast<uint64_t*>(&spr[addr - 0x70000000]);
	if (addr < 0x2000000)
		return *reinterpret_cast<uint64_t*>(&ram[addr]);
	if (addr >= 0x1C000000 && addr < 0x1C200000)
		return *reinterpret_cast<uint64_t*>(&iop_ram[addr-0x1C000000]);
	
	switch (addr)
	{
	case 0x12001000:
		return GS::ReadGSCSR();
	}

	printf("Read64 from unknown address 0x%08x\n", addr);
	exit(1);
}

uint32_t Bus::Read32(uint32_t addr)
{
	addr = Translate(addr);

	if (addr >= 0x1fc00000 && addr < 0x20000000)
		return *reinterpret_cast<uint32_t*>(&BiosRom[addr - 0x1fc00000]);
	if (addr >= 0x70000000 && addr < 0x70004000)
		return *reinterpret_cast<uint32_t*>(&spr[addr - 0x70000000]);
	if (addr < 0x2000000)
		return *reinterpret_cast<uint32_t*>(&ram[addr]);
	if (addr >= 0x1C000000 && addr < 0x1C200000)
		return *reinterpret_cast<uint32_t*>(&iop_ram[addr - 0x1C000000]);

	switch (addr)
	{
	case 0x1000f130:
	case 0x1000f400:
	case 0x1000f410:
	case 0x1000f430:
	case 0x1f80141c:
		return 0;
	case 0x1000f000:
		return INTC_STAT;
	case 0x1000f010:
		return INTC_MASK;
	case 0x1000f440:
	{
		uint8_t SOP = (MCH_RICM >> 6) & 0xF;
		uint16_t SA = (MCH_RICM >> 16) & 0xFFF;
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
	case 0x10003020:
		return GIF::ReadStat();
	case 0x1000C000:
	case 0x1000C020:
		return DMAC::ReadSIF0Channel(addr);
	case 0x1000C400:
	case 0x1000C420:
	case 0x1000C430:
		return DMAC::ReadSIF1Channel(addr);
	case 0x1000E000:
		return DMAC::ReadDCTRL();
	case 0x1000E010:
		return DMAC::ReadDSTAT();
	case 0x1000E020:
		return DMAC::ReadDPCR();
	case 0x1000F200:
		return SIF::ReadMSCOM_EE();
	case 0x1000F210:
		return SIF::ReadSMCOM();
	case 0x1000F220:
		return SIF::ReadMSFLG();
	case 0x1000F230:
		return SIF::ReadSMFLG();
	case 0x1000F520:
		return DMAC::ReadDENABLE();
	case 0x1000A000:
	case 0x1000A010:
	case 0x1000A030:
	case 0x1000A040:
	case 0x1000A050:
	case 0x1000A080:
		return DMAC::ReadGIFChannel(addr);
	case 0x12001000:
		return GS::ReadGSCSR() & 0xffffffff;
	case 0x10001000:
	case 0x10001010:
	case 0x10001810:
	case 0x10002010:
		return 0;
	}

	printf("Read32 from unknown address 0x%08x\n", addr);
	exit(1);
}

uint16_t Bus::Read16(uint32_t addr)
{
	addr = Translate(addr);

	if (addr >= 0x1fc00000 && addr < 0x20000000)
		return *reinterpret_cast<uint16_t*>(&BiosRom[addr - 0x1fc00000]);
	if (addr >= 0x70000000 && addr < 0x70004000)
		return *reinterpret_cast<uint16_t*>(&spr[addr - 0x70000000]);
	if (addr < 0x2000000)
		return *reinterpret_cast<uint16_t*>(&ram[addr]);

	switch (addr)
	{
	case 0x1f803800:
		return 0;
	}

	printf("Read16 from unknown address 0x%08x\n", addr);
	exit(1);
}

uint8_t Bus::Read8(uint32_t addr)
{
	addr = Translate(addr);

	if (addr == 0x1e104)
	{
		printf("Reading from transfers_queued\n");
	}


	if (addr >= 0x1fc00000 && addr < 0x20000000)
		return BiosRom[addr - 0x1fc00000];
	if (addr >= 0x70000000 && addr < 0x70004000)
		return spr[addr - 0x70000000];
	if (addr < 0x2000000)
		return ram[addr];
	if (addr >= 0x1C000000 && addr < 0x1C200000)
		return iop_ram[addr-0x1C000000];

	switch (addr)
	{
	case 0x1f803204:
		return 0;
	}

	printf("Read from unknown address 0x%08x\n", addr);
	exit(1);
}

void Bus::Write128(uint32_t addr, uint128_t data)
{
	EmotionEngine::MarkDirty(addr, sizeof(data));

	addr = Translate(addr);

	if (addr < 0x2000000)
	{
		*reinterpret_cast<__uint128_t*>(&ram[addr]) = data.u128;
		return;
	}
	if (addr >= 0x70000000 && addr < 0x70004000)
	{
		*reinterpret_cast<__uint128_t*>(&spr[addr - 0x70000000]) = data.u128;
		return;
	}

	if (addr >= 0x11000000 && addr < 0x11004000)
	{
		VectorUnit::WriteDataMem128(0, addr, data);
		return;
	}
	if (addr >= 0x11004000 && addr < 0x11008000)
	{
		VectorUnit::WriteDataMem128(0, addr, data);
		return;
	}
	if (addr >= 0x1100C000 && addr < 0x11010000)
	{
		VectorUnit::WriteDataMem128(1, addr, data);
		return;
	}

	switch (addr)
	{
	case 0x10004000:
		VIF::WriteVIF0FIFO(data);
		return;
	case 0x10005000:
		VIF::WriteVIF1FIFO(data);
		return;
	case 0x10006000:
		GIF::WriteFIFO(data);
		return;
	case 0x10007010:
		return;
	}

	printf("Write128 0x%lx%016lx to unknown address 0x%08x\n",
			data.u64[1], data.u64[0], addr);
	exit(1);
}

void Bus::Write64(uint32_t addr, uint64_t data)
{
	EmotionEngine::MarkDirty(addr, sizeof(data));

	addr = Translate(addr);

	if (addr >= 0x70000000 && addr < 0x70004000)
	{
		*reinterpret_cast<uint64_t*>(&spr[addr - 0x70000000]) = data;
		return;
	}
	if (addr < 0x2000000)
	{
		*reinterpret_cast<uint64_t*>(&ram[addr]) = data;
		return;
	}
	if (addr >= 0x11008000 && addr < 0x1100C000)
	{
		VectorUnit::WriteCodeMem64(1, addr, data);
		return;
	}

	switch (addr)
	{
	case 0x12001000:
		GS::WriteGSCSR(data);
		return;
	case 0x12000000:
		GS::WriteGSPMODE(data);
		return;
	case 0x12000010:
		GS::WriteGSSMODE1(data);
		return;
	case 0x12000020:
		GS::WriteGSSMODE2(data);
		return;
	case 0x12000030:
		GS::WriteGSSRFSH(data);
		return;
	case 0x12000040:
		GS::WriteGSSYNCH1(data);
		return;
	case 0x12000050:
		GS::WriteGSSYNCH2(data);
		return;
	case 0x12000060:
		GS::WriteGSSYNCV(data);
		return;
	case 0x12000070:
		GS::WriteDISPFB1(data);
		return;
	case 0x12000080:
		GS::WriteDISPLAY1(data);
		return;
	case 0x12000090:
		GS::WriteDISPFB2(data);
		return;
	case 0x120000A0:
		GS::WriteDISPLAY2(data);
		return;
	case 0x120000E0:
		GS::WriteBGCOLOR(data);
		return;
	case 0x12001010:
		GS::WriteIMR(data);
		return;
	case 0x10000800: // Timer 1
	case 0x10000810:
		return;
	}

	printf("Write64 0x%08lx to unknown address 0x%08x\n", data, addr);
	exit(1);
}

bool firstTime = true;

void Bus::Write32(uint32_t addr, uint32_t data)
{
	EmotionEngine::MarkDirty(addr, sizeof(data));

	addr = Translate(addr);

	printf("Wrote 0x%08x to 0x%08x\n", data, addr);

	if (addr == 0x10c0 && data == 0 && !firstTime)
	{
		printf("Writing 0 to 0x10c0\n");
		exit(1);
	}
	else if (addr == 0x10c0 && data == 0)
		firstTime = false;

	if (addr >= 0x70000000 && addr < 0x70004000)
	{
		*reinterpret_cast<uint32_t*>(&spr[addr - 0x70000000]) = data;
		return;
	}
	if (addr < 0x2000000)
	{
		*reinterpret_cast<uint32_t*>(&ram[addr]) = data;
		return;
	}
	if (addr >= 0x1C000000 && addr < 0x1C200000)
	{
		*reinterpret_cast<uint32_t*>(&iop_ram[addr-0x1C000000]) = data;
		return;
	}

	switch (addr)
	{
	case 0x1000f100:  // Some weird RDRAM stuff
	case 0x1000f120:
	case 0x1000f140:
	case 0x1000f150:
	case 0x1000f400:
	case 0x1000f410:
	case 0x1000f420:
	case 0x1000f450:
	case 0x1000f460:
	case 0x1000f480:
	case 0x1000f490:
	case 0x1f80141c:
		return;
	case 0x1000f000:
		printf("Writing 0x%08x to INTC_STAT\n", data);
		INTC_STAT &= ~(data);
		return;
	case 0x1000f010:
		printf("Writing 0x%08x to INTC_MASK\n", data);
		INTC_MASK = data;
		return;
	case 0x1000f500:  // EE TLB enable?
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
	// GIF
	case 0x10003000:
		GIF::WriteCtrl32(data);
		return;
	// Timers
	case 0x10000000:
	case 0x10000010:
	case 0x10000020:
	case 0x10000030:
	case 0x10000800:
	case 0x10000810:
	case 0x10000820:
	case 0x10000830:
	case 0x10001000:
	case 0x10001010:
	case 0x10001020:
	case 0x10001030:
	case 0x10001800:
	case 0x10001810:
	case 0x10001820:
	case 0x10001830:
	case 0x1000f510:
		return;
	case 0x10008000:
	case 0x10008010:
	case 0x10008030:
	case 0x10008040:
	case 0x10008050:
	case 0x10008080:
		DMAC::WriteVIF0Channel(addr, data);
		return;
	case 0x10009000:
	case 0x10009010:
	case 0x10009030:
	case 0x10009040:
	case 0x10009050:
	case 0x10009080:
		DMAC::WriteVIF1Channel(addr, data);
		return;
	case 0x1000A000:
	case 0x1000A010:
	case 0x1000A020:
	case 0x1000A030:
	case 0x1000A040:
	case 0x1000A050:
	case 0x1000A080:
		DMAC::WriteGIFChannel(addr, data);
		return;
	case 0x1000B000:
	case 0x1000B010:
	case 0x1000B030:
	case 0x1000B040:
	case 0x1000B050:
	case 0x1000B080:
		DMAC::WriteIPUFROMChannel(addr, data);
		return;
	case 0x1000B400:
	case 0x1000B410:
	case 0x1000B430:
	case 0x1000B440:
	case 0x1000B450:
	case 0x1000B480:
		DMAC::WriteIPUTOChannel(addr, data);
		return;
	case 0x1000C000:
	case 0x1000C010:
	case 0x1000C020:
	case 0x1000C030:
	case 0x1000C040:
	case 0x1000C050:
	case 0x1000C080:
		DMAC::WriteSIF0Channel(addr, data);
		return;
	case 0x1000C400:
	case 0x1000C410:
	case 0x1000C420:
	case 0x1000C430:
	case 0x1000C440:
	case 0x1000C450:
	case 0x1000C480:
		DMAC::WriteSIF1Channel(addr, data);
		return;
	case 0x1000C800:
	case 0x1000C810:
	case 0x1000C830:
	case 0x1000C840:
	case 0x1000C850:
	case 0x1000C880:
		DMAC::WriteSIF2Channel(addr, data);
		return;
	case 0x1000D000:
	case 0x1000D010:
	case 0x1000D030:
	case 0x1000D040:
	case 0x1000D050:
	case 0x1000D080:
		DMAC::WriteSPRFROMChannel(addr, data);
		return;
	case 0x1000D400:
	case 0x1000D410:
	case 0x1000D430:
	case 0x1000D440:
	case 0x1000D450:
	case 0x1000D480:
		DMAC::WriteSPRTOChannel(addr, data);
		return;
	case 0x1000E000:
		DMAC::WriteDCTRL(data);
		return;
	case 0x1000E010:
		DMAC::WriteDSTAT(data);
		return;
	case 0x1000E020:
		DMAC::WriteDPCR(data);
		return;
	case 0x1000E030:
		DMAC::WriteSQWC(data);
	case 0x1000E040:
	case 0x1000E050:
		return;
	case 0x10003810:
		VIF::WriteFBRST(0, data);
		return;
	case 0x10003820:
	case 0x10003830:
		VIF::WriteMASK(0, data);
		return;
	case 0x10003c00:
		return;
	case 0x10003c10:
		VIF::WriteFBRST(1, data);
		return;
	case 0x10002000:
	case 0x10002010:
		return;
	case 0x1000F200:
		SIF::WriteMSCOM_EE(data);
		return;
	case 0x1000F220:
		SIF::WriteMSFLG_EE(data);
		return;
	case 0x1000F230:
		SIF::WriteSMFLG_EE(data);
		return;
	case 0x1000F240:
		SIF::WriteCTRL_EE(data);
		return;
	case 0x1000F260:
		SIF::WriteBD6_EE(data);
		return;
	case 0x1000F590:
		DMAC::WriteDENABLE(data);
		return;
	case 0x12001000:
		GS::WriteGSCSR((GS::ReadGSCSR() & 0xffffffff00000000) | data);
		return;
	}

	printf("Write32 0x%08x to unknown address 0x%08x\n", data, addr);
	exit(1);
}

void Bus::Write16(uint32_t addr, uint16_t data)
{
	EmotionEngine::MarkDirty(addr, sizeof(data));

	addr = Translate(addr);

	if (addr >= 0x70000000 && addr < 0x70004000)
	{
		*reinterpret_cast<uint16_t*>(&spr[addr - 0x70000000]) = data;
		return;
	}
	if (addr < 0x2000000)
	{
		*reinterpret_cast<uint16_t*>(&ram[addr]) = data;
		return;
	}

	if (((addr & 0xFF000000) == 0x1A000000) || ((addr & 0xFFF00000) == 0x1F800000))
		return;

	printf("Write16 to unknown address 0x%08x\n", addr);
	exit(1);
}

void Bus::Write8(uint32_t addr, uint8_t data)
{
	EmotionEngine::MarkDirty(addr, sizeof(data));

	if (addr == 0x8001e104)
	{
		printf("Writing 0x%02x to transfers_queued\n", data);
	}

	addr = Translate(addr);

	if (addr >= 0x70000000 && addr < 0x70004000)
	{
		spr[addr - 0x70000000] = data;
		return;
	}
	if (addr < 0x2000000)
	{
		ram[addr] = data;
		return;
	}

	switch (addr)
	{
	case 0x1000f180:
		console << static_cast<char>(data);
		console.flush();
		return;
	}

	printf("Write8 to unknown address 0x%08x\n", addr);
	exit(1);
}

uint32_t Bus::LoadElf(std::string name)
{
	struct Elf32_Hdr
	{
		uint8_t e_ident[16];
		uint16_t e_type;
		uint16_t e_machine;
		uint32_t e_version;
		uint32_t e_entry;
		uint32_t e_phoff;
		uint32_t e_shoff;
		uint32_t e_flags;
		uint16_t e_ehsize;
		uint16_t e_phentsize;
		uint16_t e_phnum;
		uint16_t e_shentsize;
		uint16_t e_shnum;
		uint16_t e_shstrndx;
	};

	struct Elf32_Phdr 
    {
        uint32_t p_type;
        uint32_t p_offset;
        uint32_t p_vaddr;
        uint32_t p_paddr;
        uint32_t p_filesz;
        uint32_t p_memsz;
        uint32_t p_flags;
        uint32_t p_align;
    };

	std::ifstream reader;
	reader.open(name, std::ios::in | std::ios::binary);

	reader.seekg(0, std::ios::end);
	int size = reader.tellg();
	uint8_t* buffer = new uint8_t[size];

	reader.seekg(0, std::ios::beg);
	reader.read((char*)buffer, size);
	reader.close();

	auto header = *(Elf32_Hdr*)&buffer[0];
    printf("[CORE][ELF] Loading %s\n", name.c_str());
    printf("Entry: 0x%x\n", header.e_entry);
    printf("Program header start: 0x%x\n", header.e_phoff);
    printf("Section header start: 0x%x\n", header.e_shoff);
    printf("Program header entries: %d\n", header.e_phnum);
    printf("Section header entries: %d\n", header.e_shnum);
    printf("Section header names index: %d\n", header.e_shstrndx);

	for (auto i = header.e_phoff; i < header.e_phoff + (header.e_phnum*0x20); i += 0x20)
	{
		auto pheader = *(Elf32_Phdr*)&buffer[i];

		printf("Phdr: Load: 0x%08x\n", pheader.p_vaddr);

		int mem_w = pheader.p_paddr;
		for (auto file_w = pheader.p_offset; file_w < pheader.p_offset+pheader.p_filesz; file_w += 4)
		{
			uint32_t word = *(uint32_t*)&buffer[file_w];
			Write32(mem_w, word);
			mem_w += 4;
		}
	}

	return header.e_entry;
}

void Bus::TriggerEEInterrupt(int i_num)
{
	INTC_STAT |= (1 << i_num);
	if (INTC_STAT & INTC_MASK)
		EmotionEngine::SetIp0Pending();
}
