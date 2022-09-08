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

    for (int i = 0; i < 128; i++)
    {
        icache[i].tag[0] = 1 << 31;
        icache[i].tag[1] = 1 << 31;
        icache[i].lfu[0] = false;
        icache[i].lfu[1] = false;
    }
}

void EmotionEngine::Clock()
{
    Opcode instr;
    instr.full = Read32(pc, true);

    AdvancePC();

    if (instr.full == 0)
    {
        printf("nop\n");
        HandleLoadDelay();
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
        case 0x03:
            sra(instr);
            break;
        case 0x08:
            jr(instr);
            break;
        case 0x09:
            jalr(instr);
            break;
        case 0x0B:
            movn(instr);
            break;
        case 0x0F:
            printf("sync\n");
            break;
        case 0x10:
            mfhi(instr);
            break;
        case 0x12:
            mflo(instr);
            break;
        case 0x18:
            mult(instr);
            break;
        case 0x1A:
            div(instr);
            break;
        case 0x1B:
            divu(instr);
            break;
        case 0x21:
            addu(instr);
            break;
        case 0x23:
            subu(instr);
            break;
        case 0x25:
            op_or(instr);
            break;
        case 0x2b:
            sltu(instr);
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
    case 0x01:
    {
        switch (instr.i_type.rt)
        {
        case 0x00:
            bltz(instr);
            break;
        case 0x01:
            bgez(instr);
            break;
        default:
            printf("[emu/CPU]: %s: Unknown regimm instruction 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.i_type.rt);
            Application::Exit(1);
            break;
        }
    }
    break;
    case 0x02:
        j(instr);
        break;
    case 0x03:
        jal(instr);
        break;
    case 0x04:
        beq(instr);
        break;
    case 0x05:
        bne(instr);
        break;
    case 0x06:
        blez(instr);
        break;
    case 0x07:
        bgtz(instr);
        break;
    case 0x09:
        addiu(instr);
        break;
    case 0x0A:
        slti(instr);
        break;
    case 0x0B:
        sltiu(instr);
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
    case 0x14:
        beql(instr);
        break;
    case 0x15:
        bnel(instr);
        break;
    case 0x20:
        lb(instr);
        break;
    case 0x23:
        lw(instr);
        break;
    case 0x24:
        lbu(instr);
        break;
    case 0x28:
        sb(instr);
        break;
    case 0x2B:
        sw(instr);
        break;
    case 0x37:
        ld(instr);
        break;
    case 0x39:
        swc1(instr);
        break;
    case 0x3f:
        sd(instr);
        break;
    default:
        printf("[emu/CPU]: %s: Unknown instruction 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.r_type.opcode);
        Application::Exit(1);
    }

    HandleLoadDelay();

    regs[0].u64[0] = regs[0].u64[1] = 0;
}

void EmotionEngine::Dump()
{
    for (int i = 0; i < 32; i++)
        printf("[emu/CPU]: %s: %s\t->\t%s\n", __FUNCTION__, Reg(i), print_128(regs[i]));
    printf("[emu/CPU]: %s: pc\t->\t0x%08x\n", __FUNCTION__, pc-4);
    printf("[emu/CPU]: %s: hi\t->\t0x%08x\n", __FUNCTION__, hi);
    printf("[emu/CPU]: %s: lo\t->\t0x%08x\n", __FUNCTION__, lo);
}