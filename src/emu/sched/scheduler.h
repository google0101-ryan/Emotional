#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace Scheduler
{

using EventFunc = std::function<void()>;

struct Event
{
	uintmax_t cycles_from_now = 0;
	EventFunc func;
	std::string name;

	bool operator<(Event& rhs) 
	{
		return cycles_from_now < rhs.cycles_from_now;
	}
};

void InitScheduler();
void ScheduleEvent(Event event);
void CheckScheduler(int cycles);

}