// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include "dmac.hpp"

#include <cstdio>
#include <cstdlib>

namespace DMAC
{

union DN_SADR
{
    uint32_t data;
    struct
    {
        uint32_t address : 14;
        uint32_t : 17;
    };
};

union DN_CHCR
{
    uint32_t data;
    struct
    {
        uint32_t direction : 1;
        uint32_t : 1;
        uint32_t mode : 2;
        uint32_t asp : 2;
        uint32_t tte : 1;
        uint32_t tie : 1;
        uint32_t start : 1;
        uint32_t : 7;
        uint32_t tag : 16;
    };
};

struct Channel
{
    DN_SADR sadr;
    DN_CHCR chcr;
    uint32_t madr;
    uint32_t tadr;
    uint32_t asr[2];
} channels[10];

void WriteVIF0Channel(uint32_t addr, uint32_t data)
{
    switch (addr & 0xff)
    {
    case 0x00:
        channels[0].chcr.data = data;
        if (channels[0].chcr.start)
            printf("[emu/DMAC]: Starting VIF0 transfer\n");
        break;
    case 0x10:
        channels[0].madr = data;
        break;
    case 0x30:
        channels[0].tadr = data;
        break;
    case 0x40:
        channels[0].asr[0] = data;
        break;
    case 0x50:
        channels[0].asr[1] = data;
        break;
    case 0x80:
        channels[0].sadr.data = data;
        break;
    }
}

void WriteVIF1Channel(uint32_t addr, uint32_t data)
{
    switch (addr & 0xff)
    {
    case 0x00:
        channels[1].chcr.data = data;
        if (channels[1].chcr.start)
            printf("[emu/DMAC]: Starting VIF1 transfer\n");
        break;
    case 0x10:
        channels[1].madr = data;
        break;
    case 0x30:
        channels[1].tadr = data;
        break;
    case 0x40:
        channels[1].asr[0] = data;
        break;
    case 0x50:
        channels[1].asr[1] = data;
        break;
    case 0x80:
        channels[1].sadr.data = data;
        break;
    }
}
void WriteGIFChannel(uint32_t addr, uint32_t data)
{
    switch (addr & 0xff)
    {
    case 0x00:
        channels[2].chcr.data = data;
        if (channels[2].chcr.start)
            printf("[emu/DMAC]: Starting GIF transfer\n");
        break;
    case 0x10:
        channels[2].madr = data;
        break;
    case 0x30:
        channels[2].tadr = data;
        break;
    case 0x40:
        channels[2].asr[0] = data;
        break;
    case 0x50:
        channels[2].asr[1] = data;
        break;
    case 0x80:
        channels[2].sadr.data = data;
        break;
    }
}
void WriteIPUFROMChannel(uint32_t addr, uint32_t data)
{
    switch (addr & 0xff)
    {
    case 0x00:
        channels[3].chcr.data = data;
        if (channels[3].chcr.start)
            printf("[emu/DMAC]: Starting IPU_FROM transfer\n");
        break;
    case 0x10:
        channels[3].madr = data;
        break;
    case 0x30:
        channels[3].tadr = data;
        break;
    case 0x40:
        channels[3].asr[0] = data;
        break;
    case 0x50:
        channels[3].asr[1] = data;
        break;
    case 0x80:
        channels[3].sadr.data = data;
        break;
    }
}
void WriteIPUTOChannel(uint32_t addr, uint32_t data)
{
    switch (addr & 0xff)
    {
    case 0x00:
        channels[4].chcr.data = data;
        if (channels[4].chcr.start)
            printf("[emu/DMAC]: Starting IPU_TO transfer\n");
        break;
    case 0x10:
        channels[4].madr = data;
        break;
    case 0x30:
        channels[4].tadr = data;
        break;
    case 0x40:
        channels[4].asr[0] = data;
        break;
    case 0x50:
        channels[4].asr[1] = data;
        break;
    case 0x80:
        channels[4].sadr.data = data;
        break;
    }
}
void WriteSIF0Channel(uint32_t addr, uint32_t data)
{
    switch (addr & 0xff)
    {
    case 0x00:
        channels[5].chcr.data = data;
        if (channels[5].chcr.start)
            printf("[emu/DMAC]: Starting SIF0 transfer\n");
        break;
    case 0x10:
        channels[5].madr = data;
        break;
    case 0x30:
        channels[5].tadr = data;
        break;
    case 0x40:
        channels[5].asr[0] = data;
        break;
    case 0x50:
        channels[5].asr[1] = data;
        break;
    case 0x80:
        channels[5].sadr.data = data;
        break;
    }
}
void WriteSPRFROMChannel(uint32_t addr, uint32_t data)
{
    switch (addr & 0xff)
    {
    case 0x00:
        channels[8].chcr.data = data;
        if (channels[8].chcr.start)
            printf("[emu/DMAC]: Starting SPR_FROM transfer\n");
        break;
    case 0x10:
        channels[8].madr = data;
        break;
    case 0x30:
        channels[8].tadr = data;
        break;
    case 0x40:
        channels[8].asr[0] = data;
        break;
    case 0x50:
        channels[8].asr[1] = data;
        break;
    case 0x80:
        channels[8].sadr.data = data;
        break;
    }   
}
void WriteSPRTOChannel(uint32_t addr, uint32_t data)
{
    switch (addr & 0xff)
    {
    case 0x00:
        channels[9].chcr.data = data;
        if (channels[9].chcr.start)
            printf("[emu/DMAC]: Starting SPR_TO transfer\n");
        break;
    case 0x10:
        channels[9].madr = data;
        break;
    case 0x30:
        channels[9].tadr = data;
        break;
    case 0x40:
        channels[9].asr[0] = data;
        break;
    case 0x50:
        channels[9].asr[1] = data;
        break;
    case 0x80:
        channels[9].sadr.data = data;
        break;
    }
}

union DSTAT
{
	uint32_t value;
	struct
	{
		uint32_t channel_irq : 10; /* Clear with 1 */
		uint32_t : 3;
		uint32_t dma_stall : 1; /* Clear with 1 */
		uint32_t mfifo_empty : 1; /* Clear with 1 */
		uint32_t bus_error : 1; /* Clear with 1 */
		uint32_t channel_irq_mask : 10; /* Reverse with 1 */
		uint32_t : 3;
		uint32_t stall_irq_mask : 1; /* Reverse with 1 */
		uint32_t mfifo_irq_mask : 1; /* Reverse with 1 */
		uint32_t : 1;
	};
	/* If you notice above the lower 16bits are cleared when 1 is written to them
	   while the upper 16bits are reversed. So I'm making this struct to better
	   implement this behaviour */
	struct
	{
		uint32_t clear : 16;
		uint32_t reverse : 16;
	};
} stat;

uint32_t ctrl;
uint32_t dpcr;
uint32_t sqwc;

void WriteDSTAT(uint32_t data)
{
    stat.clear &= ~(data & 0xffff);
    stat.reverse ^= (data >> 16);
}

uint32_t ReadDSTAT()
{
    return stat.value;
}

void WriteDCTRL(uint32_t data)
{
    ctrl = data;
}

void WriteDPCR(uint32_t data)
{
    dpcr = data;
}

uint32_t ReadDPCR()
{
    return dpcr;
}

void WriteSQWC(uint32_t data)
{
    sqwc = data;
}
}