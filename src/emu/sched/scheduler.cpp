#include "scheduler.h"
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
	Event infinite_event;
	infinite_event.cycles_from_now = std::numeric_limits<uintmax_t>::max() - global_cycles;
	infinite_event.name = "Infinite Event (Unreachable)";
	ScheduleEvent(infinite_event);
}

bool CompareEvents(const Event& a, const Event& b)
{
	return a.cycles_from_now > b.cycles_from_now;
}

void ScheduleEvent(Event event)
{
	event.cycles_from_now += global_cycles;
	event_queue.emplace_back(event);
	std::push_heap(event_queue.begin(), event_queue.end(), CompareEvents);
}

void CheckScheduler(int cycles)
{
	global_cycles += cycles;

	while (event_queue.front().cycles_from_now <= global_cycles)
	{
		event_queue.front().func();
		std::pop_heap(event_queue.begin(), event_queue.end());
		event_queue.pop_back();
	}

	if (event_queue.size() > 1)
		printf("Next event is %lu cycles from now\n", event_queue.front().cycles_from_now - global_cycles);
}

}