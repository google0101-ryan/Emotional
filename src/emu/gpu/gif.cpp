#include "gif.hpp"

#include <cstdio>
#include <queue>
#include <emu/sched/scheduler.h>

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

void ProcessGIFData()
{
	fifo.pop();
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
	printf("[emu/GIF]: Adding 0x%lx%016lx to GIF FIFO\n", data.u64[1], data.u64[0]);
	if (fifo.size() < 16)
	{
		fifo.push(data);
	}
	
	if (!event_scheduled)
	{
		Scheduler::Event event;
		event.cycles_from_now = curTransferSize;
		event.name = "GIF Data Processing";
		event.func = ProcessGIFData;

		event_scheduled = true;

		Scheduler::ScheduleEvent(event);
	}
}
}