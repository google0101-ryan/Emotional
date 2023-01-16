#include "vif.h"

#include <cstdio>
#include <cstdlib>
#include <queue>
#include <emu/sched/scheduler.h>

bool vif1_event_scheduled = false;
bool vif0_event_scheduled = false;

void VIF::WriteFBRST(int vif_num, uint32_t data)
{
	if (data & 1)
	{
		printf("[emu/VIF%d]: Reset\n", vif_num);
	}
}

void VIF::WriteMASK(int vif_num, uint32_t data)
{
}

std::queue<uint32_t> vif1_fifo, vif0_fifo;

struct VIF_DATA
{
	uint16_t cycle;
	uint16_t mask;
	uint16_t mode;
	uint16_t itop;
} vif1, vif0;

void HandleVIF1Data()
{
	uint32_t data = vif1_fifo.front();
	vif1_fifo.pop();

	uint8_t cmd = (data >> 24) & 0xff;
	uint16_t imm = data & 0xffff;

	switch (cmd)
	{
	case 0x01:
		printf("[emu/VIF1]: STCYCL 0x%04x\n", imm);
		vif1.cycle = imm;
		break;
	default:
		printf("[emu/VIF1]: Unknown CMD 0x%02x\n", cmd);
		exit(1);
	}

	if (!vif1_fifo.empty())
	{
		Scheduler::Event event;
		event.cycles_from_now = 2;
		event.func = HandleVIF1Data;
		event.name = "VIF1 Data Handler";
	}
	else
		vif1_event_scheduled = false;
}

void VIF::WriteVIF1FIFO(uint128_t data)
{
	for (int i = 0; i < 4; i++)
	{
		vif1_fifo.push(data.u32[i]);
	}
	
	if (!vif1_event_scheduled)
	{
		Scheduler::Event event;
		// Most of these events should be handled ASAP
		event.cycles_from_now = 0;
		event.func = HandleVIF1Data;
		event.name = "VIF1 Data Handler";

		Scheduler::ScheduleEvent(event);
		vif1_event_scheduled = true;
	}
}

void HandleVIF0Data()
{
	uint32_t data = vif0_fifo.front();
	vif0_fifo.pop();

	uint8_t cmd = (data >> 24) & 0xff;
	uint16_t imm = data & 0xffff;

	switch (cmd)
	{
	case 0x00:
		printf("[emu/VIF0]: NOP\n");
		break;
	case 0x01:
		printf("[emu/VIF0]: STCYCL 0x%04x\n", imm);
		vif0.cycle = imm;
		break;
	case 0x04:
		printf("[emu/VIF0]: ITOP 0x%04x\n", imm);
		vif0.itop = imm;
		break;
	case 0x05:
		printf("[emu/VIF0]: STMOD 0x%04x\n", imm);
		vif0.mode = imm;
		break;
	case 0x20:
		printf("[emu/VIF0]: STMASK 0x%04x\n", imm);
		vif0.mask = imm;
		break;
	default:
		printf("[emu/VIF0]: Unknown CMD 0x%02x\n", cmd);
		exit(1);
	}

	if (!vif0_fifo.empty())
	{
		Scheduler::Event event;
		event.cycles_from_now = 2;
		event.func = HandleVIF0Data;
		event.name = "VIF0 Data Handler";

		Scheduler::ScheduleEvent(event);
		vif0_event_scheduled = true;
	}
	else
	{
		vif0_event_scheduled = false;
	}
}

void VIF::WriteVIF0FIFO(uint128_t data)
{

	for (int i = 0; i < 4; i++)
	{
		printf("[emu/VIF0]: Adding 0x%08x to VIF0 FIFO\n", data.u32[i]);
		vif0_fifo.push(data.u32[i]);
	}
	
	if (!vif0_event_scheduled)
	{
		Scheduler::Event event;
		// Most of these events should be handled ASAP
		event.cycles_from_now = 0;
		event.func = HandleVIF0Data;
		event.name = "VIF0 Data Handler";

		Scheduler::ScheduleEvent(event);
		vif0_event_scheduled = true;
	}
}
