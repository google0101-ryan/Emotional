#include "System.h"
#include <emu/memory/Bus.h>
#include <emu/cpu/ee/EmotionEngine.h>

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
	EmotionEngine::Reset();
}

void System::Run()
{
	while (1)
		EmotionEngine::Clock();
}

void System::Dump()
{
	EmotionEngine::Dump();
}
