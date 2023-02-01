// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include "System.h"
#include <emu/memory/Bus.h>
#include <emu/cpu/ee/EmotionEngine.h>
#include <emu/sched/scheduler.h>
#include <emu/cpu/ee/vu.h>
#include <emu/cpu/iop/cpu.h>

#include <chrono> // NOLINT [build/c++11]
#include <iostream>

Scheduler::Event vsync_event;

std::chrono::steady_clock::time_point first_tp;
uint64_t frame_count = 0;

std::chrono::duration<double> uptime()
{
	if (first_tp == std::chrono::steady_clock::time_point{})
		return std::chrono::duration<double>{0};

	return std::chrono::steady_clock::now() - first_tp;
}

double fps()
{
	const double uptime_sec = uptime().count();

	if (uptime_sec == 0)
		return 0;

	return frame_count / uptime_sec;
}

void HandleVsync()
{
	// printf("FPS: %f\n", fps());
	Scheduler::ScheduleEvent(vsync_event);
	frame_count++;
}

void System::LoadBios(std::string biosName)
{
	std::ifstream file(biosName, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		printf("[emu/Sys]: Couldn't open file %s for reading\n", biosName.c_str());
		exit(1);
	}

	size_t fSize = file.tellg();
	file.seekg(0, std::ios::beg);

	uint8_t* rom = new uint8_t[fSize];

	file.read(reinterpret_cast<char*>(rom), fSize);

	Bus::LoadBios(rom);

	delete[] rom;

	printf("[emu/Sys]: Loaded BIOS %s\n", biosName.c_str());
}

void System::Reset()
{
	Scheduler::InitScheduler();
	EmotionEngine::Reset();

	IOP_MANAGEMENT::Reset();

	vsync_event.func = HandleVsync;
	vsync_event.name = "VSYNC handler";
	vsync_event.cycles_from_now = 4920115;
	Scheduler::ScheduleEvent(vsync_event);

	first_tp = std::chrono::steady_clock::now();
}

void System::Run()
{
	EmotionEngine::Clock();
}

void System::Dump()
{
	EmotionEngine::Dump();
	Bus::Dump();
	VectorUnit::Dump();
	IOP_MANAGEMENT::Dump();
}
