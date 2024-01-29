// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include "System.h"
#include <emu/memory/Bus.h>
#include <emu/cpu/ee/EmotionEngine.h>
#include <emu/sched/scheduler.h>
#include <emu/cpu/ee/vu.h>
#include <emu/cpu/iop/cpu.h>
#include <emu/gpu/gs.h>

#include <chrono> // NOLINT [build/c++11]
#include <iostream>
#include <ctime>

std::ofstream fps_file;

Scheduler::Event vblank_start_event;
Scheduler::Event vblank_end_event;
Scheduler::Event hblank_event;

std::chrono::steady_clock::time_point first_tp;
uint64_t frame_count = 0;

std::chrono::duration<double> uptime()
{
	if (first_tp == std::chrono::steady_clock::time_point{})
		return std::chrono::duration<double>{0};

	return std::chrono::steady_clock::now() - first_tp;
}

clock_t current_ticks, delta_ticks;
clock_t fps_ = 0;

double fps()
{
	const double uptime_sec = uptime().count();

	if (uptime_sec == 0)
		return 0;

	return frame_count / uptime_sec;
}

void HandleVblankStart()
{
	Scheduler::ScheduleEvent(vblank_start_event);

	GS::SetVblankStart(true);
	GS::UpdateOddFrame();

	if (GS::VSIntEnabled())
		Bus::TriggerEEInterrupt(2);
}

void HandleVblankEnd()
{
	// printf("FPS: %f\n", fps());
	frame_count++;
	GS::UpdateFPS(fps());

	Scheduler::ScheduleEvent(vblank_end_event);
	
	GS::SetVblankStart(false);

	if (GS::VSIntEnabled())
		Bus::TriggerEEInterrupt(3);
}

void HandleHblank()
{
	Scheduler::ScheduleEvent(hblank_event);

	GS::SetHblank(true);

	if (GS::HSIntEnabled())
		Bus::TriggerEEInterrupt(0);
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

	vblank_start_event.func = HandleVblankStart;
	vblank_start_event.name = "VBLANK start handler";
	vblank_start_event.cycles_from_now = 4489019;
	Scheduler::ScheduleEvent(vblank_start_event);

	vblank_end_event.func = HandleVblankEnd;
	vblank_end_event.name = "VBLANK end handler";
	vblank_end_event.cycles_from_now = 4920115;
	Scheduler::ScheduleEvent(vblank_end_event);

	fps_file.open("fps.txt");

	first_tp = std::chrono::steady_clock::now();
}

void System::Run()
{
	while (1)
	{
		size_t cycles = Scheduler::GetNextTimestamp();

		printf("Running for a max of %ld cycles\n", cycles);

		int true_cycles = EmotionEngine::Clock(cycles);
		IOP_MANAGEMENT::Clock(true_cycles / 8);

		printf("Actual block took %ld cycles\n", true_cycles);

		Scheduler::CheckScheduler(true_cycles);
	}
}

void System::Dump()
{
	EmotionEngine::Dump();
	Bus::Dump();
	VectorUnit::Dump();
	IOP_MANAGEMENT::Dump();
	GS::DumpVram();
}
