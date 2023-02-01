// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include <emu/cpu/ee/dmac.hpp>

#include <emu/memory/Bus.h>
#include <emu/sched/scheduler.h>
#include <emu/dev/sif.h>

#include <cstdio>
#include <cstdlib>
#include <cassert>

namespace DMAC
{


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
	uint32_t qwc;
    uint32_t asr[2];
} channels[10];

uint32_t ctrl;
uint32_t d_enable = 0x1201;

union DMATag
{
	__uint128_t value;

	struct
	{
		__uint128_t qwc : 16;
		__uint128_t : 10;
		__uint128_t prio_control : 2;
		__uint128_t tag_id : 3;
		__uint128_t irq : 1;
		__uint128_t addr : 31;
		__uint128_t mem_sel : 1;
		__uint128_t data_transfer : 64;
	};
};

const char* REG_NAMES[] =
{
	"CHCR",
	"MADR",
	"QWC",
	"TADR",
	"ASR0",
	"ASR1",
	"",
	"",
	"",
	"SADR",
};

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

bool sif0_ongoing_transfer = false, fetching_sif0_tag = true;
DMATag sif0_tag;

void HandleSIF0Transfer()
{
	auto& c = channels[5];

	fetching_sif0_tag = true;

	if (!sif0_ongoing_transfer)
		return;

	if (fetching_sif0_tag)
	{
		if (SIF::FIFO0_size() >= 2)
		{
			printf("Fetching DMATag\n");
			exit(1);
		}

		sif0_ongoing_transfer = true;
	}
	else
	{
	}

	if (sif0_ongoing_transfer)
	{
		Scheduler::Event sif0_evt;
		sif0_evt.cycles_from_now = 2;
		sif0_evt.func = HandleSIF0Transfer;
		sif0_evt.name = "SIF0 DMA transfer";

		Scheduler::ScheduleEvent(sif0_evt);
	}
	else
	{
		printf("[emu/DMAC]: Transfer ended on SIF0 channel\n");

		stat.channel_irq |= (1 << 5);

		if (stat.channel_irq & stat.channel_irq_mask)
		{
			printf("[emu/DMAC]: Fire interrupt\n");
			exit(1);
		}

		c.chcr.start = 0;
	}
}

void WriteSIF0Channel(uint32_t addr, uint32_t data)
{
	printf("[emu/DMAC]: Writing 0x%08x to %s of SIF0 channel\n", data, REG_NAMES[(addr >> 4) & 0xf]);

    switch (addr & 0xff)
    {
    case 0x00:
        channels[5].chcr.data = data;
        if (channels[5].chcr.start && (ctrl & 1))
		{
            printf("[emu/DMAC]: Starting SIF0 transfer\n");

			// We schedule the transfer 1 cycle from now
			// This is because the DMAC ticks at half the speed of the EE
			Scheduler::Event sif0_evt;
			sif0_evt.cycles_from_now = 1;
			sif0_evt.func = HandleSIF0Transfer;
			sif0_evt.name = "SIF0 DMA transfer";

			Scheduler::ScheduleEvent(sif0_evt);

			sif0_ongoing_transfer = true;
		}
        break;
    case 0x10:
        channels[5].madr = data;
        break;
	case 0x20:
		channels[5].qwc = data & 0xffff;
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

bool fetching_tag = true, ongoing_transfer = false;

DMATag tag;

void HandleSIF1Transfer()
{
	auto& c = channels[6];

	if (!ongoing_transfer)
		return;

	if (fetching_tag)
	{
		tag.value = Bus::Read128(c.tadr).u128;

		printf("[emu/DMAC]: Found tag on SIF1 %s at 0x%08x\n", print_128({tag.value}), c.tadr);

		c.qwc = tag.qwc;

		switch (tag.tag_id)
		{
		case 0:
			c.madr = tag.addr;
			c.tadr += 16;
			tag.irq = true;  // End the transfer and fire an irq
			break;
		case 3:
			c.madr = tag.addr;
			c.tadr += 16;
			break;
		default:
			printf("[emu/DMAC]: unknown tag mode %d\n", tag.tag_id);
			exit(1);
		}

		fetching_tag = false;

		int cycles_til_transfer = tag.qwc * 2;  // Each qword transfered takes two cycles, so we transfer all at once that many cycles later

		Scheduler::Event sif1_evt;
		sif1_evt.cycles_from_now = cycles_til_transfer;
		sif1_evt.func = HandleSIF1Transfer;
		sif1_evt.name = "SIF1 DMA transfer";

		Scheduler::ScheduleEvent(sif1_evt);

		if (c.chcr.tte)
		{
			uint128_t qword = {tag.value};

			for (int i = 0; i < 4; i++)
			{
				SIF::WriteFIFO1(qword.u32[i]);
			}
		}
	}
	else
	{
		for (int i = 0; i < tag.qwc; i++)
		{
			uint128_t qword = Bus::Read128(c.madr);
			c.madr += 16;

			printf("[emu/DMAC]: Transfering qword %s\n", print_128(qword));

			for (int i = 0; i < 4; i++)
			{
				SIF::WriteFIFO1(qword.u32[i]);
			}
		}

		c.qwc = 0;

		fetching_tag = true;

		if (tag.irq)
			ongoing_transfer = false;
	}

	if (ongoing_transfer)
	{
		Scheduler::Event sif1_evt;
		sif1_evt.cycles_from_now = 2;
		sif1_evt.func = HandleSIF1Transfer;
		sif1_evt.name = "SIF1 DMA transfer";

		Scheduler::ScheduleEvent(sif1_evt);
	}
	else
	{
		printf("[emu/DMAC]: Transfer ended on SIF1 channel\n");

		stat.channel_irq |= (1 << 6);

		if (stat.channel_irq & stat.channel_irq_mask)
		{
			printf("[emu/DMAC]: Fire interrupt\n");
			exit(1);
		}

		c.chcr.start = 0;
	}
}

void WriteSIF1Channel(uint32_t addr, uint32_t data)
{
	printf("[emu/DMAC]: Writing 0x%08x to %s of SIF1 channel\n", data, REG_NAMES[(addr >> 4) & 0xf]);

    switch (addr & 0xff)
    {
    case 0x00:
        channels[6].chcr.data = data;
		if (channels[6].chcr.start && (ctrl & 1))
		{
            printf("[emu/DMAC]: Starting SIF1 transfer\n");

			// We schedule the transfer 1 cycle from now
			// This is because the DMAC ticks at half the speed of the EE
			Scheduler::Event sif1_evt;
			sif1_evt.cycles_from_now = 1;
			sif1_evt.func = HandleSIF1Transfer;
			sif1_evt.name = "SIF1 DMA transfer";

			Scheduler::ScheduleEvent(sif1_evt);

			ongoing_transfer = true;
		}
        break;
    case 0x10:
        channels[6].madr = data;
        break;
	case 0x20:
		channels[6].qwc = data;
		break;
    case 0x30:
        channels[6].tadr = data;
        break;
    case 0x40:
        channels[6].asr[0] = data;
        break;
    case 0x50:
        channels[6].asr[1] = data;
        break;
    case 0x80:
        channels[6].sadr.data = data;
        break;
    }
}

void WriteSIF2Channel(uint32_t addr, uint32_t data)
{
    switch (addr & 0xff)
    {
    case 0x00:
        channels[7].chcr.data = data;
        if (channels[7].chcr.start)
            printf("[emu/DMAC]: Starting SIF2 transfer\n");
        break;
    case 0x10:
        channels[7].madr = data;
        break;
    case 0x30:
        channels[7].tadr = data;
        break;
    case 0x40:
        channels[7].asr[0] = data;
        break;
    case 0x50:
        channels[7].asr[1] = data;
        break;
    case 0x80:
        channels[7].sadr.data = data;
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

uint32_t ReadSIF0Channel(uint32_t addr)
{
	switch (addr & 0xff)
    {
	case 0x00:
		return channels[5].chcr.data;
	case 0x20:
		return channels[5].qwc;
	}
}

uint32_t ReadSIF1Channel(uint32_t addr)
{
	switch (addr & 0xff)
    {
	case 0x00:
		return channels[6].chcr.data;
	case 0x20:
		return channels[6].qwc;
	case 0x30:
		return channels[6].tadr;
	}
}

uint32_t ReadDENABLE()
{
	return d_enable;
}

void WriteDENABLE(uint32_t data)
{
	d_enable = data;
}

uint32_t dpcr;
uint32_t sqwc;

void WriteDSTAT(uint32_t data)
{
	printf("[emu/DMAC]: Writing 0x%08x to D_STAT\n", data);

    stat.clear &= ~(data & 0xffff);
    stat.reverse ^= (data >> 16);

	printf("0x%08x\n", stat.value);
}

uint32_t ReadDSTAT()
{
    return stat.value;
}

void WriteDCTRL(uint32_t data)
{
    ctrl = data;
}

uint32_t ReadDCTRL()
{
	return ctrl;
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

}  // namespace DMAC
