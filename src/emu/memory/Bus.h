// (c) Copyright 2022 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstdint>
#include <util/uint128.h>

#include <emu/cpu/iop/dma.h>
#include <emu/dev/cdvd.h>
#include <emu/dev/sif.h>

uint32_t Translate(uint32_t addr);
extern uint8_t* BiosRom;
extern uint8_t* iop_ram;

namespace Bus
{

void LoadBios(uint8_t* data);

void Dump();

uint128_t Read128(uint32_t addr);
uint64_t Read64(uint32_t addr);
uint32_t Read32(uint32_t addr);
uint16_t Read16(uint32_t addr);
uint8_t Read8(uint32_t addr);

void Write128(uint32_t addr, uint128_t data);
void Write64(uint32_t addr, uint64_t data);
void Write32(uint32_t addr, uint32_t data);
void Write16(uint32_t addr, uint16_t data);
void Write8(uint32_t addr, uint8_t data);

extern uint32_t I_MASK, I_STAT, I_CTRL;

template<typename T>
T iop_read(uint32_t addr)
{
	addr = Translate(addr);

	if (addr >= 0x1fc00000 && addr < 0x20000000)
		return *(T*)&BiosRom[addr - 0x1fc00000];
	if (addr < 0x200000)
		return *(T*)&iop_ram[addr];
	if (addr >= 0x1E000000 && addr < 0x1F000000)
		return 0;

	switch (addr)
	{
	case 0x1d000010:
		return SIF::ReadSMCOM();
	case 0x1d000020:
		return SIF::ReadMSFLG();
	case 0x1d000030:
		return SIF::ReadSMFLG();
	case 0x1d000040:
		return SIF::ReadCTRL();
	case 0x1d000060:
		return 0;
	case 0x1f402005:
		return CDVD::ReadNStatus();
	case 0x1f80100C:
	case 0x1f801010:
	case 0x1f801400:
	case 0x1f801450:
		return 0;
	case 0x1f801074:
		return I_MASK;
	case 0x1f8010f0:
		return IopDma::ReadDPCR();
	case 0x1f8010f4:
		return IopDma::ReadDICR();
	case 0x1f801078:
		return I_CTRL;
	case 0x1f801570:
		return IopDma::ReadDPCR2();
	case 0x1f801574:
		return IopDma::ReadDICR2();
	case 0x1f801578:
		return IopDma::ReadDMACEN();
	// Timers
	case 0x1F801100 ... 0x1F801108:
	case 0x1F801110 ... 0x1F801118:
	case 0x1F801120 ... 0x1F801128:
	case 0x1F801480 ... 0x1F801488:
	case 0x1F801490 ... 0x1F801498:
	case 0x1F8014A0 ... 0x1F8014A8:
		return 0;
	}

	printf("[emu/IopBus]: Read from unknown addr 0x%08x\n", addr);
	exit(1);
}

template<typename T>
void iop_write(uint32_t addr, T data)
{
	addr = Translate(addr);
	
	if (addr < 0x200000)
	{
		*(T*)&iop_ram[addr] = data;
		return;
	}

	switch (addr)
	{
	case 0x1d000010:
		SIF::WriteSMCOM_IOP(data);
		return;
	case 0x1d000030:
		SIF::WriteSMFLG_IOP(data);
		return;
	case 0x1d000040:
		SIF::WriteCTRL_IOP(data);
		return;
	case 0x1f801004:
	case 0x1f801008:
	case 0x1f80100C:
	case 0x1f801010:
	case 0x1f801014:
	case 0x1f801018:
	case 0x1f80101C:
	case 0x1f801020:
	case 0x1f801060:
	case 0x1f802070:
	case 0x1f801400:
	case 0x1f801404:
	case 0x1f801408:
	case 0x1f80140C:
	case 0x1f801410:
	case 0x1f801414:
	case 0x1f801418:
	case 0x1f80141C:
	case 0x1f801420:
	case 0x1f801450:
	case 0x1f801560:
	case 0x1f801564:
	case 0x1f801568:
	case 0x1f80156C:
	case 0x1f8015f0:
	case 0x1ffe0130:
	case 0x1ffe0140:
		return;
	case 0x1ffe0144:
		printf("[emu/IOP]: Scratchpad start 0x%08x\n", data);
		return;
	case 0x1f801074:
		I_MASK = data;
		return;
	case 0x1f801078:
		I_CTRL = data;
		return;
	case 0x1f8010f0:
		IopDma::WriteDPCR(data);
		return;
	case 0x1f8010f4:
		IopDma::WriteDICR(data);
		return;
	case 0x1f801570:
		IopDma::WriteDPCR2(data);
		return;
	case 0x1F801574:
		IopDma::WriteDICR2(data);
		return;
	case 0x1f801578:
		IopDma::WriteDMACEN(data);
		return;
	case 0x1f801080 ... 0x1f80108C:
	case 0x1f801090 ... 0x1f80109C:
	case 0x1f8010A0 ... 0x1f8010AC:
	case 0x1f8010B0 ... 0x1f8010BC:
	case 0x1f8010C0 ... 0x1f8010CC:
	case 0x1f8010D0 ... 0x1f8010DC:
	case 0x1f8010E0 ... 0x1f8010EC:
		IopDma::WriteChannel(addr, data);
		return;
	case 0x1f801500 ... 0x1f80150C:
	case 0x1f801510 ... 0x1f80151C:
	case 0x1f801520 ... 0x1f80152C:
	case 0x1f801530 ... 0x1f80153C:
	case 0x1f801540 ... 0x1f80154C:
	case 0x1f801550 ... 0x1f80155C:
		IopDma::WriteNewChannel(addr, data);
		return;
	// Timers
	case 0x1F801100 ... 0x1F801108:
	case 0x1F801110 ... 0x1F801118:
	case 0x1F801120 ... 0x1F801128:
	case 0x1F801480 ... 0x1F801488:
	case 0x1F801490 ... 0x1F801498:
	case 0x1F8014A0 ... 0x1F8014A8:
		return;
	}

	printf("[emu/IopBus]: Write to unknown addr 0x%08x\n", addr);
	exit(1);
}

}