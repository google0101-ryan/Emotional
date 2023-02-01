// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

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

	bool operator<(const Event& rhs) const
	{
		return cycles_from_now < rhs.cycles_from_now;
	}
};

void InitScheduler();
void ScheduleEvent(Event event);
void CheckScheduler(int cycles);

}  // namespace Scheduler
