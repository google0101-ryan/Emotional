#include "app/Application.h"
#include "util/uint128.h"
#include <bits/stdint-uintn.h>
#include <cstring>
#include <emu/cpu/EmotionEngine.h>
#include <emu/cpu/opcode.h>

EmotionEngine::EmotionEngine(Bus* bus)
: bus(bus)
{
    memset(regs, 0, sizeof(regs));
    pc = 0xBFC00000;
    next_pc = pc + 4;
}

void EmotionEngine::Clock()
{
    Opcode instr;
    instr.full = Read32Instr(pc);

    switch (instr.r_type.opcode)
    {
    default:
        printf("[emu/CPU]: %s: Unknown instruction 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.r_type.opcode);
        Application::Exit(1);
    }
}

void EmotionEngine::Dump()
{
    for (int i = 0; i < 32; i++)
        printf("[emu/CPU]: %s: r%d\t->\t%s\n", __FUNCTION__, i, print_128(regs[i]));
}