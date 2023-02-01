// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include <emu/sched/scheduler.h>

#include <emu/cpu/iop/cpu.h>

#include <vector>
#include <algorithm>
#include <limits>
#include <cinttypes>

namespace Scheduler
{

uintmax_t global_cycles = 0;

std::vector<Event> event_queue;

void InitScheduler()
{
	std::make_heap(event_queue.begin(), event_queue.end());
}

bool CompareEvents(const Event& a, const Event& b)
{
	return a.cycles_from_now > b.cycles_from_now;
}

void ScheduleEvent(Event event)
{
	event.cycles_from_now += global_cycles;
	if (event.cycles_from_now < global_cycles)  // Overflow detected
	{
		printf("[emu/Sched]: Overflow found\n");
		for (auto& e : event_queue)
		{
			e.cycles_from_now = e.cycles_from_now - global_cycles;
		}
		global_cycles = 0;
	}
	event_queue.emplace_back(event);
	std::push_heap(event_queue.begin(), event_queue.end(), CompareEvents);
}

int iop_cycles = 0;

void CheckScheduler(int cycles)
{
	global_cycles += cycles;

	IOP_MANAGEMENT::Clock(cycles / 4);

	if (event_queue.empty())
		return;

	while (event_queue.front().cycles_from_now <= global_cycles)
	{
		auto e = event_queue.front();
		std::pop_heap(event_queue.begin(), event_queue.end());
		event_queue.pop_back();
		e.func();
	}
}

}  // namespace Scheduler
