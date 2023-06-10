// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include <emu/gpu/gif.hpp>
#include <emu/gpu/gs.h>

#include <emu/sched/scheduler.h>

#include <cstdio>
#include <queue>

namespace GIF
{

bool CanTransfer = true;
uint32_t dataCount = 0;
uint32_t regCount = 0;

uint32_t curTransferSize = 0;

bool event_scheduled = false;

std::queue<uint128_t> fifo;

union GIF_CTRL
{
	uint32_t data;
	struct
	{
		uint32_t path3_masked_mode : 1;
		uint32_t path3_masked_vif1 : 1;
		uint32_t intermittent : 1;
		uint32_t freeze : 1;
		uint32_t : 1;
		uint32_t path3_int : 1;
		uint32_t path3_queued : 1;
		uint32_t path2_queued : 1;
		uint32_t path1_queued : 1;
		uint32_t output_path : 1;
		uint32_t active_path : 2;
		uint32_t dir : 1;
		uint32_t : 11;
		uint32_t data_count : 5;
		uint32_t : 3;
	};
} ctrl;


void Reset()
{
	dataCount = 0;
	regCount = 0;
	std::queue<uint128_t> empty_fifo;
	fifo.swap(empty_fifo);
}

union GIFTag
{
	__uint128_t value;
	struct
	{
		__uint128_t nloop : 15,
		eop : 1,
		: 30,
		prim_en : 1,
		prim_data : 11,
		fmt : 2,
		nregs : 4,
		reg : 64;
	};
} tag;

int data_count = 0;
int regs_left = 0;

void ProcessPacked(uint128_t qword)
{
	int curr_reg = tag.nregs - regs_left;
	uint64_t regs = tag.reg;
	uint32_t desc = (regs >> 4 * curr_reg) & 0xf;
	uint64_t data1 = qword.u64[0];
	uint64_t data2 = qword.u64[1];

	
	switch (desc)
	{
	case 0x01:
	{
		uint8_t r = qword.u128 & 0xff;
		uint8_t g = (qword.u128 >> 32) & 0xff;
		uint8_t b = (qword.u128 >> 64) & 0xff;
		uint8_t a = (qword.u128 >> 96) & 0xff;
		GS::WriteRGBAQ(r, g, b, a);
		break;
	}
	case 0x04:
	{
		uint32_t x = data1 & 0xffff;
		uint32_t y = (data1 >> 32) & 0xffff;
		uint32_t z = (data2 >> 4) & 0xFFFFFF;
		bool disable_drawing = (data2 >> (111 - 64)) & 1;
		uint8_t f = (data2 >> (100 - 64)) & 0xff;
		printf("Write vertex (%d, %d, %d) to %s (%s) (%d)\n", x >> 4, y >> 4, z, desc == 0x04 ? "xyzf2" : "xyzf3", print_128(qword), disable_drawing);
		GS::WriteXYZF(x, y, z, f, false);
		break;
	}
	case 0x05:
	{
		uint32_t x = data1 & 0xffff;
		uint32_t y = (data1 >> 32) & 0xffff;
		uint32_t z = data2 & 0xFFFFFFFF;
		printf("Write vertex (%d, %d, %d) to %s (%s)\n", x >> 4, y >> 4, z, desc == 0x04 ? "xyz2" : "xyz3", print_128(qword));
		GS::WriteXYZF(x, y, z, 0.0f, false);
		break;
	}
	case 0x0e:
		desc = qword.u64[1];
		GS::WriteRegister(desc, qword.u64[0]);
		break;
	default:
		printf("Write %s to unknown register GIF packed mode 0x%02x\n", print_128(qword), desc);
		exit(1);
	}
}

void ProcessREGLIST(uint128_t qword)
{
	for (int i = 0; i < 2; i++)
	{
		uint64_t reg_offset = (tag.nregs - regs_left) << 2;
		uint8_t reg = (tag.reg >> reg_offset) & 0xf;

		if (reg != 0xE)
			GS::WriteRegister(reg, qword.u64[i]);
		
		regs_left--;
		if (!regs_left)
		{
			regs_left = tag.nregs;
			data_count--;

			if (!data_count && !i)
				return;
		}
	}
}

void ProcessGIFData()
{
	while (!fifo.empty())
	{
		if (!data_count)
		{
			tag.value = fifo.front().u128;
			fifo.pop();
			data_count = tag.nloop;
			regs_left = tag.nregs;

			printf("[emu/GIF]: Found tag %s\n", print_128({tag.value}));

			if (tag.prim_en)
				GS::WritePRIM(tag.prim_data);
		}
		else
		{
			uint128_t qword = fifo.front();


			switch (tag.fmt)
			{
			case 0:
			{
				ProcessPacked(qword);
				regs_left--;
				if (!regs_left)
				{
					regs_left = tag.nregs;
					data_count--;
				}
				break;
			}
			case 1:
				ProcessREGLIST(qword);
				break;
			case 2:
			case 3:
				GS::WriteHWReg(qword.u128);
				GS::WriteHWReg(qword.u128 >> 64);
				data_count--;
				break;
			default:
				printf("Unknown GIFTAG format %d\n", tag.fmt);
				exit(1);
			}

			printf("%ld qwords left in packet\n", data_count);

			fifo.pop();
		}
	}

	if (!fifo.empty())
	{
		Scheduler::Event event;
		event.cycles_from_now = 5;
		event.name = "GIF Data Processing";
		event.func = ProcessGIFData;

		Scheduler::ScheduleEvent(event);
	}
	else
		event_scheduled = false;
}

void WriteCtrl32(uint32_t data)
{
	CanTransfer = data & (1 << 3);
	if (data & 1)
	{
		Reset();
		printf("[emu/GIF]: Reset\n");
	}
}

uint32_t ReadStat()
{
	ctrl.data_count = fifo.size();
	return ctrl.data;
}

void WriteFIFO(uint128_t data)
{
	fifo.push(data);

	if (!event_scheduled)
	{
		Scheduler::Event event;
		event.cycles_from_now = curTransferSize*4;
		event.name = "GIF Data Processing";
		event.func = ProcessGIFData;

		event_scheduled = true;

		Scheduler::ScheduleEvent(event);
	}
}

}  // namespace GIF
