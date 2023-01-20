#include "scheduler.h"
#include <vector>
#include <algorithm>
#include <limits>
#include <cinttypes>
#include <emu/cpu/iop/cpu.h>

namespace Scheduler
{

uintmax_t global_cycles = 0;

std::vector<Event> event_queue;

void InitScheduler()
{
	std::make_heap(event_queue.begin(), event_queue.end());
	// Event infinite_event;
	// infinite_event.cycles_from_now = std::numeric_limits<uintmax_t>::max() - global_cycles;
	// infinite_event.name = "Infinite Event (Unreachable)";
	// ScheduleEvent(infinite_event);
}

bool CompareEvents(const Event& a, const Event& b)
{
	return a.cycles_from_now > b.cycles_from_now;
}

void ScheduleEvent(Event event)
{
	event.cycles_from_now += global_cycles;
	if (event.cycles_from_now < global_cycles) // Overflow detected
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

	// if (!event_queue.empty())
	// 	printf("[emu/Sched]: Next event is in %d cycles\n", event_queue.front().cycles_from_now - global_cycles);
}

}