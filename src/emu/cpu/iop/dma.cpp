#include "dma.h"
#include <cstdio>
#include <cstdlib>
#include <emu/sched/scheduler.h>
#include <emu/dev/sif.h>
#include <emu/memory/Bus.h>
#include <cassert>

union DICR
{
	uint32_t value;
	struct
	{
		uint32_t tag : 13;
		uint32_t : 3;
		uint32_t mask : 6;
		uint32_t : 2;
		uint32_t flags : 6;
		uint32_t : 2;
	};
} dicr, dicr2 = {0};

uint32_t dpcr;
uint32_t dpcr2;
bool dmacen = true;

union DN_CHCR
{
	uint32_t value;
	struct
	{
		uint32_t direction : 1;
		uint32_t increment : 1;
		uint32_t : 6;
		uint32_t bit_8 : 1;
		uint32_t transfer_mode : 2;
		uint32_t : 5;
		uint32_t chop_dma : 3;
		uint32_t : 1;
		uint32_t chop_cpu : 3;
		uint32_t : 1;
		uint32_t running : 1;
		uint32_t : 3;
		uint32_t trigger : 1;
		uint32_t : 3;
	};
};

union DN_BCR
{
	uint32_t value;
	struct
	{
		uint32_t size : 16;
		uint32_t count : 16;
	};
};

struct DMAChannels
{
	uint32_t madr;
	DN_BCR bcr;
	DN_CHCR chcr;
	uint32_t tadr;
} channels[13];

union DMATag
{
	uint64_t value;
	struct
	{
		uint64_t start_addr : 24;
		uint64_t : 6;
		uint64_t irq : 1;
		uint64_t end : 1;
		uint64_t size : 24;
		uint64_t : 8;
	};
};

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

bool sif1_transfer_running = false;

DMATag sif1_tag;

void HandleSIF1Transfer()
{
	auto& c = channels[10];

	// Handle any leftovers that may believe we're still transfering
	if (!sif1_transfer_running)
		return;

	if (c.bcr.count)
	{
		while (c.bcr.count)
		{
			if (SIF::FIFO1_size())
			{
				auto data = SIF::ReadAndPopSIF1();

				Bus::iop_write<uint32_t>(c.madr, data);
				c.madr += 4;
				c.bcr.count--;
			}
			else
				break;
		}
		
			
		if (sif1_tag.irq || sif1_tag.end)
		{
			dicr2.flags |= (1 << 3);

			if (dicr2.flags & dicr2.mask)
				Bus::TriggerIOPInterrupt(3);
			
			sif1_transfer_running = false;
			c.chcr.trigger = c.chcr.running = 0;
		}
	}
	else
	{
		if (SIF::FIFO1_size() >= 4)
		{
			uint32_t data[2];
			for (int i = 0; i < 2; i++)
				data[i] = SIF::ReadAndPopSIF1();
			
			SIF::ReadAndPopSIF1();
			SIF::ReadAndPopSIF1();
			
			sif1_tag.value = *(uint64_t*)data;

			printf("[emu/IopDma]: Found SIF1 DMATag 0x%08lx: Start address 0x%08x, size %d words (%d, %d)\n", sif1_tag.value, sif1_tag.start_addr, sif1_tag.size, sif1_tag.end, sif1_tag.irq);

			c.madr = sif1_tag.start_addr;
			c.bcr.count = (sif1_tag.size + 3) & 0xfffffffc;
		}
	}

	if (sif1_transfer_running)
	{
		Scheduler::Event evt;
		evt.name = "SIF1Transfer";
		evt.func = HandleSIF1Transfer;
		evt.cycles_from_now = 8*c.bcr.count;
		if (evt.cycles_from_now == 0) evt.cycles_from_now = 8;
		
		Scheduler::ScheduleEvent(evt);
	}
}



bool sif0_transfer_running = false;

DMATag sif0_tag;

void HandleSIF0Transfer()
{
	auto& c = channels[9];
	
	// Handle any leftovers that may believe we're still transfering
	if (!sif0_transfer_running)
		return;

	printf("[emu/IopDma]: Transfer of mode %d, %d blocks from 0x%08x\n", c.chcr.transfer_mode, c.bcr.count, c.madr);

	if (c.bcr.count)
	{
		while (c.bcr.count)
		{
			uint32_t data = Bus::iop_read<uint32_t>(c.madr);
			SIF::WriteFIFO0(data);
			c.madr += 4;
			c.bcr.count--;
			printf("\r%08d blocks remaining", c.bcr.count);
		}
		printf("\n");
		
			
		if (sif0_tag.irq || sif0_tag.end)
		{
			dicr2.flags |= (1 << 2);

			if (dicr2.flags & dicr2.mask)
				Bus::TriggerIOPInterrupt(3);
			
			sif0_transfer_running = false;
			c.chcr.trigger = c.chcr.running = 0;
		}
	}
	else
	{
		sif0_tag.value = Bus::iop_read<uint64_t>(c.tadr);
		c.madr = sif0_tag.start_addr;

		printf("[emu/IopDma]: Tag read from 0x%08x\n", c.tadr);
		
		c.bcr.count = (sif0_tag.size + 3) & 0xfffffffc;
		c.tadr += 8;

		if (c.chcr.bit_8)
		{
			SIF::WriteFIFO0(Bus::iop_read<uint32_t>(c.tadr));
			SIF::WriteFIFO0(Bus::iop_read<uint32_t>(c.tadr + 4));

			c.tadr += 8;
		}

		sif0_transfer_running = true;

		printf("[emu/IopDma]: Found SIF0 DMATag 0x%08lx: Start address 0x%08x, size %d words (%d, %d)\n", sif0_tag.value, sif0_tag.start_addr, sif0_tag.size, sif0_tag.irq, sif0_tag.end);
	}

	if (sif0_transfer_running)
	{
		Scheduler::Event evt;
		evt.name = "SIF0Transfer";
		evt.func = HandleSIF0Transfer;
		evt.cycles_from_now = 8*c.bcr.count;
		if (evt.cycles_from_now == 0) evt.cycles_from_now = 8;
		
		Scheduler::ScheduleEvent(evt);
	}
}

bool spu2_done = false;

void HandleSPU2Transfer()
{
	auto& c = channels[7];

	bool schedule_new = true;

	if (c.bcr.count)
		c.bcr.count--;
	else if (spu2_done)
	{
		Bus::spu2_stat |= 0x80;
		c.chcr.running = c.chcr.trigger = 0;
		spu2_done = false;

		dicr2.flags |= (1 << 0);

		if (dicr2.flags & dicr2.mask)
			Bus::TriggerIOPInterrupt(3);
		
		schedule_new = false;
	}
	else
	{
		assert(channels[7].chcr.transfer_mode == 1);
		spu2_done = true;
	}

	if (schedule_new)
	{
		Scheduler::Event evt;
		evt.name = "SPU2 DMA Transfer";
		evt.func = HandleSPU2Transfer;
		evt.cycles_from_now = 8*c.bcr.count;
		if (evt.cycles_from_now == 0) evt.cycles_from_now = 8;
		
		Scheduler::ScheduleEvent(evt);
	}
}


void HandleRunningChannel(int chan, DMAChannels& c)
{
	if (chan == 10)
	{
		sif1_transfer_running = true;

		Scheduler::Event evt;
		evt.name = "SIF1Transfer";
		evt.func = HandleSIF1Transfer;
		evt.cycles_from_now = 8;
		
		Scheduler::ScheduleEvent(evt);
	}
	else if (chan == 9)
	{
		sif0_transfer_running = true;

		Scheduler::Event evt;
		evt.name = "SIF0Transfer";
		evt.func = HandleSIF0Transfer;
		evt.cycles_from_now = 8;
		
		Scheduler::ScheduleEvent(evt);
	}
	else if (chan == 4)
		channels[4].chcr.running = channels[4].chcr.trigger = 0;
	else if (chan == 7)
	{
		sif0_transfer_running = true;

		Scheduler::Event evt;
		evt.name = "SPU2 DMA Transfer";
		evt.func = HandleSPU2Transfer;
		evt.cycles_from_now = 8;
		
		Scheduler::ScheduleEvent(evt);
	}
	else
	{
		assert(0);
	}
}

void IopDma::WriteDMACEN(uint32_t data)
{
	dmacen = data & 1;

	if (dmacen)
	{
		for (int i = 0; i < 13; i++)
		{
			auto& c = channels[i];

			if (c.chcr.running)
			{
				HandleRunningChannel(i, c);
			}
		}
	}
}

void IopDma::WriteDICR(uint32_t data)
{
	auto& irq = dicr;
	auto flags = irq.flags;

	irq.value = data;
	irq.flags = flags & ~((data >> 24) & 0x7f);
}

void IopDma::WriteDICR2(uint32_t data)
{
	printf("[emu/IopDma]: Writing 0x%08x to DICR2\n", data);
	auto& irq = dicr2;
	auto flags = irq.flags;

	irq.value = data;
	irq.flags = flags & ~((data >> 24) & 0x7f);
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

	printf("[emu/IopDma]: Writing 0x%08x to %s of channel %d (0x%08x)\n", data, REGS[reg], channel, addr);

	switch (reg)
	{
	case 0x0:
		channels[channel].madr = data;
		break;
	case 0x1:
		channels[channel].bcr.value = data;
		break;
	case 0x2:
		channels[channel].chcr.value = data;
		break;
	case 0x3:
		channels[channel].tadr = data;
		break;
	}

	if ((channels[channel].chcr.running || channels[channel].chcr.trigger) && dmacen)
	{
		printf("[emu/IopDma]: Starting transfer on channel %d\n", channel);
		HandleRunningChannel(channel, channels[channel]);
	}
}

void IopDma::WriteNewChannel(uint32_t addr, uint32_t data)
{
	int channel = (addr >> 4) & 0xf;
	int reg = addr & 0xf;
	reg /= 4;

	channel += 7;

	printf("[emu/IopDma]: Writing 0x%08x to %s of channel %d (0x%08x)\n", data, REGS[reg], channel, addr);

	switch (reg)
	{
	case 0x0:
		channels[channel].madr = data;
		break;
	case 0x1:
		channels[channel].bcr.value = data;
		break;
	case 0x2:
		channels[channel].chcr.value = data;
		break;
	case 0x3:
		channels[channel].tadr = data;
		break;
	}

	if ((channels[channel].chcr.running || channels[channel].chcr.trigger) && dmacen)
	{
		printf("[emu/IopDma]: Starting transfer on channel %d\n", channel);
		HandleRunningChannel(channel, channels[channel]);
	}
}

uint32_t IopDma::ReadChannel(uint32_t addr)
{
	int channel = (addr >> 4) & 0xf;
	int reg = addr & 0xf;
	reg /= 4;

	channel -= 8;

	switch (reg)
	{
	case 0x0:
		return channels[channel].madr;
	case 0x1:
		return channels[channel].bcr.value;
	case 0x2:
		return channels[channel].chcr.value;
	case 0x3:
		return channels[channel].tadr;
	}
}

uint32_t IopDma::ReadNewChannel(uint32_t addr)
{
	int channel = (addr >> 4) & 0xf;
	int reg = addr & 0xf;
	reg /= 4;

	channel += 7;

	switch (reg)
	{
	case 0x0:
		return channels[channel].madr;
	case 0x1:
		return channels[channel].bcr.value;
	case 0x2:
		return channels[channel].chcr.value;
	case 0x3:
		return channels[channel].tadr;
	}
}

uint32_t IopDma::ReadDMACEN()
{
	return dmacen;
}

uint32_t IopDma::ReadDICR2()
{
	return dicr2.value;
}

uint32_t IopDma::ReadDICR()
{
	return dicr.value;
}

uint32_t IopDma::ReadDPCR2()
{
	return dpcr2;
}

uint32_t IopDma::ReadDPCR()
{
	return dpcr;
}
