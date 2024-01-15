#include "RegAllocator.h"
#include "EEJitx64.h"
#include <emu/cpu/ee/EmotionEngine.h>

struct HostRegister
{
    bool allocated;
    GuestRegister mapping;
    int used = 0;
} regs[16];

size_t RegAllocatorX64::GetRegOffset(GuestRegister reg)
{
    switch (reg)
    {
    case GuestRegister::REG_AT: return offsetof(EmotionEngine::ProcessorState, regs[1]);
    case GuestRegister::REG_V0: return offsetof(EmotionEngine::ProcessorState, regs[2]);
    case GuestRegister::REG_V1: return offsetof(EmotionEngine::ProcessorState, regs[3]);
    case GuestRegister::REG_A0: return offsetof(EmotionEngine::ProcessorState, regs[4]);
    case GuestRegister::REG_A1: return offsetof(EmotionEngine::ProcessorState, regs[5]);
    case GuestRegister::REG_A2: return offsetof(EmotionEngine::ProcessorState, regs[6]);
    case GuestRegister::REG_A3: return offsetof(EmotionEngine::ProcessorState, regs[7]);
    case GuestRegister::REG_T0: return offsetof(EmotionEngine::ProcessorState, regs[8]);
    case GuestRegister::REG_T1: return offsetof(EmotionEngine::ProcessorState, regs[9]);
    case GuestRegister::REG_T2: return offsetof(EmotionEngine::ProcessorState, regs[10]);
    case GuestRegister::REG_T3: return offsetof(EmotionEngine::ProcessorState, regs[11]);
    case GuestRegister::REG_T4: return offsetof(EmotionEngine::ProcessorState, regs[12]);
    case GuestRegister::REG_T5: return offsetof(EmotionEngine::ProcessorState, regs[13]);
    case GuestRegister::REG_T6: return offsetof(EmotionEngine::ProcessorState, regs[14]);
    case GuestRegister::REG_T7: return offsetof(EmotionEngine::ProcessorState, regs[15]);
    case GuestRegister::REG_S0: return offsetof(EmotionEngine::ProcessorState, regs[16]);
    case GuestRegister::REG_S1: return offsetof(EmotionEngine::ProcessorState, regs[17]);
    case GuestRegister::REG_S2: return offsetof(EmotionEngine::ProcessorState, regs[18]);
    case GuestRegister::REG_S3: return offsetof(EmotionEngine::ProcessorState, regs[19]);
    case GuestRegister::REG_S4: return offsetof(EmotionEngine::ProcessorState, regs[20]);
    case GuestRegister::REG_S5: return offsetof(EmotionEngine::ProcessorState, regs[21]);
    case GuestRegister::REG_S6: return offsetof(EmotionEngine::ProcessorState, regs[22]);
    case GuestRegister::REG_S7: return offsetof(EmotionEngine::ProcessorState, regs[23]);
    case GuestRegister::REG_T8: return offsetof(EmotionEngine::ProcessorState, regs[24]);
    case GuestRegister::REG_T9: return offsetof(EmotionEngine::ProcessorState, regs[25]);
    case GuestRegister::REG_K0: return offsetof(EmotionEngine::ProcessorState, regs[26]);
    case GuestRegister::REG_K1: return offsetof(EmotionEngine::ProcessorState, regs[27]);
    case GuestRegister::REG_GP: return offsetof(EmotionEngine::ProcessorState, regs[28]);
    case GuestRegister::REG_SP: return offsetof(EmotionEngine::ProcessorState, regs[29]);
    case GuestRegister::REG_FP: return offsetof(EmotionEngine::ProcessorState, regs[30]);
    case GuestRegister::REG_RA: return offsetof(EmotionEngine::ProcessorState, regs[31]);
    case GuestRegister::REG_COP0_INDEX ... GuestRegister::REG_COP0_CONFIG:
        return offsetof(EmotionEngine::ProcessorState, cop0_regs)+((reg - COP0_OFFS) * sizeof(uint32_t));
	case LO: return offsetof(EmotionEngine::ProcessorState, lo);
	case HI: return offsetof(EmotionEngine::ProcessorState, hi);
	case LO1: return offsetof(EmotionEngine::ProcessorState, lo1);
	case HI1: return offsetof(EmotionEngine::ProcessorState, hi1);
    default:
        printf("[REGALLOC_X64]: Couldn't find offset of unknown guest register %d (%d)\n", (int)reg, (int)reg - 32);
        exit(1);
    }
}

int RegAllocatorX64::GetHostReg(GuestRegister reg, bool dest)
{
    for (int i = 0; i < 16; i++)
    {
        if (regs[i].mapping == reg)
        {
            regs[i].used++;
            return i;
        }
    }

    // No reg found, replacement time

    // First look for unallocated registers
    for (int i = 0; i < 16; i++)
    {
        if (!regs[i].allocated)
        {
            regs[i].allocated = true;
            regs[i].mapping = reg;
            regs[i].used = 1;
            if (!dest)
                EEJitX64::JitLoadReg(reg, i);
            return i;
        }
    }

    int index = 0;
    int hitsLeast = -1;

    // Find the least used register
    for (int i = 0; i < 16; i++)
    {
        if (regs[i].used > hitsLeast)
        {
            hitsLeast = regs[i].used;
            index = i;
        }
    }

    // Write back the register
    EEJitX64::JitStoreReg(regs[index].mapping);

    regs[index].mapping = reg;
    regs[index].used = 1;
    if (!dest)
        EEJitX64::JitLoadReg(reg, index);
    return index;
}

void RegAllocatorX64::DoWriteback()
{
    for (int i = 0; i < 16; i++)
    {
        if (regs[i].allocated && regs[i].mapping != GuestRegister::NONE)
        {
            EEJitX64::JitStoreReg(regs[i].mapping);
            regs[i].allocated = false;
            regs[i].mapping = GuestRegister::NONE;
            regs[i].used = 0;
        }
    }
}

void RegAllocatorX64::InvalidateRegister(HostRegisters reg)
{
	if (regs[reg].allocated && regs[reg].mapping != GuestRegister::NONE)
	{
		EEJitX64::JitStoreReg(regs[reg].mapping);
		regs[reg].allocated = false;
		regs[reg].mapping = GuestRegister::NONE;
		regs[reg].used = 0;
	}
}

void RegAllocatorX64::Reset()
{
    for (int i = 0; i < 16; i++)
    {
        regs[i].allocated = false;
        regs[i].mapping = GuestRegister::NONE;
        regs[i].used = 0;
    }

    // Mark RSP, RBP, RAX, RDI, RSI, RCX, and R8 as used
    // RSP is used for the stack (and it should never be overwritten)
    // RBP is used to point to the processor state
    // RAX is used for return values
    // RDI and RSI are used for passing values to functions
    // R8 holds the current value of pc
    // RCX is for function pointers
    regs[RSP].allocated = true;
    regs[RSP].mapping = GuestRegister::NONE;
    regs[RSP].used = -1;
    
    regs[RBP].allocated = true;
    regs[RBP].mapping = GuestRegister::NONE;
    regs[RBP].used = -1;
    
    regs[RAX].allocated = true;
    regs[RAX].mapping = GuestRegister::NONE;
    regs[RAX].used = -1;
    
    regs[RCX].allocated = true;
    regs[RCX].mapping = GuestRegister::NONE;
    regs[RCX].used = -1;
    
    regs[RDI].allocated = true;
    regs[RDI].mapping = GuestRegister::NONE;
    regs[RDI].used = -1;
    
    regs[RSI].allocated = true;
    regs[RSI].mapping = GuestRegister::NONE;
    regs[RSI].used = -1;

    regs[R8].allocated = true;
    regs[R8].mapping = GuestRegister::NONE;
    regs[R8].used = -1;
}

RegAllocatorX64::RegAllocatorX64()
{
    Reset();
}
