#pragma once

#include <stddef.h>
#include <stdint.h>

#include "GuestRegister.h"

class RegAllocatorX64
{
public:
    RegAllocatorX64();

    int GetHostReg(GuestRegister reg, bool dest = false);
    size_t GetRegOffset(GuestRegister reg);
    void DoWriteback();
};