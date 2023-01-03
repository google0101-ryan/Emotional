#pragma once

#include <cstdint>
#include <array>

class RegisterAllocator
{
private:
	std::array<bool, 8> used_registers;
	// Due to how 8-bit registers map, we have a seperate array
	std::array<bool, 8> used_registers_8;

	enum HostRegisters64
	{
		RAX,
		RCX,
		RDX,
		RBX,
		RSP,
		RBP,
		RSI,
		RDI
	};
public:
	RegisterAllocator();
	void Reset();

	int AllocHostRegister();
};