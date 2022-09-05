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

    cop0_regs[15] = 0x2E20;
}

void EmotionEngine::Clock()
{
    Opcode instr;
    instr.full = Read32Instr(pc);

    AdvancePC();

    if (instr.full == 0)
    {
        printf("nop\n");
        return;
    }

    switch (instr.r_type.opcode)
    {
    case 0x00:
    {
        switch (instr.r_type.func)
        {
        case 0x00:
            sll(instr);
            break;
        case 0x08:
            jr(instr);
            break;
        case 0x09:
            jalr(instr);
            break;
        case 0x0F:
            printf("sync\n");
            break;
        case 0x18:
            mult(instr);
            break;
        case 0x25:
            op_or(instr);
            break;
        case 0x2d:
            daddu(instr);
            break;
        default:
            printf("[emu/CPU]: %s: Unknown special instruction 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.r_type.func);
            Application::Exit(1);
            break;
        }
    }
    break;
    case 0x03:
        j(instr);
        break;
    case 0x04:
        beq(instr);
        break;
    case 0x05:
        bne(instr);
        break;
    case 0x09:
        addiu(instr);
        break;
    case 0x0A:
        slti(instr);
        break;
    case 0x0C:
        andi(instr);
        break;
    case 0x0D:
        ori(instr);
        break;
    case 0x0F:
        lui(instr);
        break;
    case 0x10:
    {
        switch (instr.r_type.rs)
        {
        case 0:
            mfc0(instr);
            break;
        case 4:
            mtc0(instr);
            break;
        case 0x10:
            printf("TODO: tlbwi\n");
            break;
        default:
            printf("[emu/CPU]: %s: Unknown cop0 instruction 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.r_type.rs);
            Application::Exit(1);
            break;
        }
    }
    break;
    case 0x2B:
        sw(instr);
        break;
    case 0x3f:
        sd(instr);
        break;
    default:
        printf("[emu/CPU]: %s: Unknown instruction 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.r_type.opcode);
        Application::Exit(1);
    }

    regs[0].u64[0] = regs[0].u64[1] = 0;
}

void EmotionEngine::Dump()
{
    for (int i = 0; i < 32; i++)
        printf("[emu/CPU]: %s: %s\t->\t%s\n", __FUNCTION__, Reg(i), print_128(regs[i]));
}