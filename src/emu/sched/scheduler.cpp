// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include <emu/sched/scheduler.h>

#include <emu/cpu/iop/cpu.h>

#include <vector>
#include <algorithm>
#include <limits>
#include <cinttypes>
#include "scheduler.h"

namespace Scheduler
{

uintmax_t global_cycles = 0;

std::vector<Event> event_queue;

bool CompareEvents(const Event& a, const Event& b)
{
	return a.cycles_from_now > b.cycles_from_now;
}

void InitScheduler()
{
	std::make_heap(event_queue.begin(), event_queue.end(), CompareEvents);
}

void ScheduleEvent(Event event)
{
	event.cycles_from_now += global_cycles;
	event_queue.emplace_back(event);
	std::push_heap(event_queue.begin(), event_queue.end(), CompareEvents);
}

int iop_cycles = 0;
uint64_t next_tp = 0;

void CheckScheduler(uint64_t cycles)
{
	global_cycles += cycles;

	if (!next_tp && !event_queue.empty())
	{
		next_tp = event_queue.front().cycles_from_now;
	}

	if (global_cycles < next_tp)
		return;

	if (event_queue.empty())
		return;
		
	while (event_queue.front().cycles_from_now <= global_cycles)
	{
		auto e = event_queue.front();
		std::pop_heap(event_queue.begin(), event_queue.end(), CompareEvents);
		event_queue.pop_back();
		e.func();
		next_tp = 0;
		return;
	}
}

// Gets the cycles until the next event
// Used for ticking the EE
size_t GetNextTimestamp()
{
	if (event_queue.size() == 0)
		return 30;
	int64_t ts = event_queue.front().cycles_from_now - global_cycles;
	if (ts < 0)
		return 30;
	return event_queue.front().cycles_from_now - global_cycles;
}
}  // namespace Scheduler
