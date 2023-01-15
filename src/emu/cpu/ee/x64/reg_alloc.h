// (c) Copyright 2022 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstdint>
#include <array>

class RegisterAllocator
{
private:
	std::array<bool, 8> used_registers;
	// Due to how 8-bit registers map, we have a seperate array
	std::array<bool, 8> used_registers_8;
	std::array<bool, 8> used_xmm_registers;

public:
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
	
	RegisterAllocator();
	void Reset();

	int AllocHostRegister();
	int AllocHostXMMRegister();
	void MarkRegUsed(int reg);
};