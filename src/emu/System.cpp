// (c) Copyright 2022 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include "System.h"
#include <emu/memory/Bus.h>
#include <emu/cpu/ee/EmotionEngine.h>
#include <emu/sched/scheduler.h>

Scheduler::Event vsync_event;

void HandleVsync()
{
	printf("Vsync\n");
	Scheduler::ScheduleEvent(vsync_event);
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

	file.read((char*)rom, fSize);

	Bus::LoadBios(rom);
	
	delete[] rom;

	printf("[emu/Sys]: Loaded BIOS %s\n", biosName.c_str());
}

void System::Reset()
{
	Scheduler::InitScheduler();
	EmotionEngine::Reset();

	vsync_event.func = HandleVsync;
	vsync_event.name = "VSYNC handler";
	vsync_event.cycles_from_now = 4920115;
	Scheduler::ScheduleEvent(vsync_event);
}

void System::Run()
{
	while (1)
	{
		int cycles = EmotionEngine::Clock();
		Scheduler::CheckScheduler(cycles);
	}
}

void System::Dump()
{
	EmotionEngine::Dump();
	Bus::Dump();
}
