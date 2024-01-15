#pragma once

#include <stddef.h>
#include <stdint.h>

#include "GuestRegister.h"

enum HostRegisters
{
    RAX,
    RCX,
    RDX,
    RBX,
    RSP,
    RBP,
    RSI,
    RDI,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15
};

class RegAllocatorX64
{
public:
    RegAllocatorX64();

    int GetHostReg(GuestRegister reg, bool dest = false);
    size_t GetRegOffset(GuestRegister reg);
    void DoWriteback();
	void InvalidateRegister(HostRegisters reg);
    void Reset();
};