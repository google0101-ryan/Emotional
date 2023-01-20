#include "dma.h"
#include <cstdio>

uint32_t dpcr;
uint32_t dpcr2;
bool dmacen = true;
uint32_t dicr = 0;
uint32_t dicr2 = 0;

union DN_CHCR
{
	uint32_t data;
	struct
	{
		uint32_t direction : 1;
		uint32_t madr_inc : 1;
		uint32_t : 6;
		uint32_t mode_specific : 1;
		uint32_t mode : 2;
		uint32_t : 5;
		uint32_t chopping_dma_size : 3;
		uint32_t : 1;
		uint32_t chopping_cpu_size : 3;
		uint32_t : 1;
		uint32_t start : 1;
		uint32_t : 3;
		uint32_t force_transfer : 1;
		uint32_t forced_burst_pause : 1;
		uint32_t bus_snooping : 1;
		uint32_t iLink_thingy : 1;
	};
};

struct DMAChannels
{
	uint32_t madr;
	uint32_t bcr;
	DN_CHCR chcr;
	uint32_t tadr;
} channels[13];

void IopDma::WriteDPCR(uint32_t data)
{
	dpcr = data;
	printf("[emu/IopDma]: Writing 0x%08x to DPCR\n", data);
}

void IopDma::WriteDPCR2(uint32_t data)
{
	dpcr2 = data;
	printf("[emu/IopDma]: Writing 0x%08x to DPCR2\n", data);
}

void IopDma::WriteDMACEN(uint32_t data)
{
	printf("[emu/IopDma]: Writing 0x%08x to DMACEN\n", data);
	dmacen = data & 1;
}

void IopDma::WriteDICR(uint32_t data)
{
	dicr = data;
}

void IopDma::WriteDICR2(uint32_t data)
{
	dicr2 = data;
}

const char* REGS[] =
{
	"MADR",
	"BCR",
	"CHCR",
	"TADR"
};

void IopDma::WriteChannel(uint32_t addr, uint32_t data)
{
	int channel = (addr >> 4) & 0xf;
	int reg = addr & 0xf;
	reg /= 4;

	channel -= 0x8;

	printf("Writing 0x%08x to %s of channel %d\n", data, REGS[reg], channel);

	switch (addr)
	{
	case 0x0:
		channels[channel].madr = data;
		return;
	case 0x1:
		channels[channel].bcr = data;
		return;
	case 0x2:
		channels[channel].chcr.data = data;
		return;
	case 0x3:
		channels[channel].tadr = data;
		return;
	}

	if (channels[channel].chcr.start && dmacen)
		printf("[emu/IopDma]: Starting transfer on channel %d\n", channel);
}

void IopDma::WriteNewChannel(uint32_t addr, uint32_t data)
{
	int channel = (addr >> 4) & 0xf;
	int reg = addr & 0xf;
	reg /= 4;

	channel += 8;

	printf("Writing 0x%08x to %s of channel %d\n", data, REGS[reg], channel);

	switch (addr)
	{
	case 0x0:
		channels[channel].madr = data;
		return;
	case 0x1:
		channels[channel].bcr = data;
		return;
	case 0x2:
		channels[channel].chcr.data = data;
		return;
	case 0x3:
		channels[channel].tadr = data;
		return;
	}

	if (channels[channel].chcr.start && dmacen)
		printf("[emu/IopDma]: Starting transfer on channel %d\n", channel);
}

uint32_t IopDma::ReadDMACEN()
{
	return dmacen;
}

uint32_t IopDma::ReadDICR2()
{
	return dicr2;
}

uint32_t IopDma::ReadDICR()
{
	return dicr;
}

uint32_t IopDma::ReadDPCR2()
{
	return dpcr2;
}

uint32_t IopDma::ReadDPCR()
{
	return dpcr;
}
