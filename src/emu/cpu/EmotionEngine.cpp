#include "app/Application.h"
#include "util/uint128.h"
#include <bits/stdint-uintn.h>
#include <cstring>
#include <emu/cpu/EmotionEngine.h>
#include <emu/cpu/opcode.h>
#include <assert.h>

EmotionEngine::EmotionEngine(Bus* bus, VectorUnit* vu0)
: bus(bus),
vu0(vu0)
{
    memset(regs, 0, sizeof(regs));
    pc = 0xBFC00000;
    
    next_instr = {};
    next_instr.full = bus->read<uint32_t>(pc);
    next_instr.pc = pc;
	pc += 4;

    cop0_regs[15] = 0x2E20;

    for (int i = 0; i < 128; i++)
    {
        icache[i].tag[0] = 1 << 31;
        icache[i].tag[1] = 1 << 31;
        icache[i].lfu[0] = false;
        icache[i].lfu[1] = false;
    }
}

void EmotionEngine::Clock(int cycles)
{
    int cycles_to_execute = cycles;
    for (int cycle = 0; cycle < cycles; cycle++)
    {
        instr = next_instr;

        fetch_next();

        switch (instr.r_type.opcode)
        {
        case 0x00:
        {
            switch (instr.r_type.func)
            {
            case 0x00:
                sll(instr);
                break;
            case 0x02:
                srl(instr);
                break;
            case 0x03:
                sra(instr);
                break;
            case 0x04:
                sllv(instr);
                break;
            case 0x06:
                srlv(instr);
                break;
            case 0x07:
                srav(instr);
                break;
            case 0x08:
                jr(instr);
                break;
            case 0x09:
                jalr(instr);
                break;
            case 0x0a:
                movz(instr);
                break;
            case 0x0B:
                movn(instr);
                break;
            case 0x0F:
                //printf("sync\n");
                break;
            case 0x10:
                mfhi(instr);
                break;
            case 0x12:
                mflo(instr);
                break;
            case 0x14:
                dsllv(instr);
                break;
            case 0x17:
                dsrav(instr);
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
            case 0x24:
                op_and(instr);
                break;
            case 0x25:
                op_or(instr);
                break;
            case 0x27:
                op_nor(instr);
                break;
            case 0x2a:
                slt(instr);
                break;
            case 0x2b:
                sltu(instr);
                break;
            case 0x2d:
                daddu(instr);
                break;
            case 0x38:
                dsll(instr);
                break;
            case 0x3A:
                dsrl(instr);
                break;
            case 0x3c:
                dsll32(instr);
                break;
            case 0x3e:
                dsrl32(instr);
                break;
            case 0x3f:
                dsra32(instr);
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
        case 0x0E:
            xori(instr);
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
                //printf("TODO: tlbwi\n");
                break;
            default:
                printf("[emu/CPU]: %s: Unknown cop0 instruction 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.r_type.rs);
                Application::Exit(1);
                break;
            }
        }
        break;
        case 0x11:
        {
            switch (instr.i_type.rs)
            {
            case 0:
                mfc1(instr);
                break;
            case 4:
                mtc1(instr);
                break;
            case 6:
                break;
            case 0x10:
                switch (instr.full & 0x3F)
                {
                case 0x18:
                    adda(instr);
                    break;
                default:
                    printf("[emu/CPU]: %s: Unknown cop1.s instruction 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.full & 0x3F);
                    Application::Exit(1);
                    break;
                }
            break;
            default:
                printf("[emu/CPU]: %s: Unknown cop1 instruction 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.i_type.rs);
                Application::Exit(1);
                break;
            }
        }
        break;
        case 0x12:
        {
            switch (instr.r_type.rs)
            {
            case 0x01:
            {
                int rd = instr.r_type.rd;
                int rt = instr.r_type.rt;
                for (int i = 0; i < 4; i++)
                {
                    regs[rd].u32[i] = vu0->GetGprU(rt, i);
                }
                //printf("qmfc2 %s, f%d\n", Reg(rd), rt);
            }
            break;
            case 0x02:
                regs[instr.r_type.rd].u32[0] = (int32_t)vu0->cfc(instr.r_type.rt);
                //printf("cfc2 %s, %d\n", Reg(instr.r_type.rd), instr.r_type.rt);
                break;
            case 0x05:
            {
                int rd = instr.r_type.rd;
                int rt = instr.r_type.rt;

                for (int i = 0; i < 4; i++)
                {
                    vu0->SetGprU(rd, i, regs[rt].u32[i]);
                }
                //printf("qmfc2 %s, f%d\n", Reg(rt), rd);
            }
            break;
            case 0x06:
                if (instr.r_type.rd == 16)
                    vu0->ctc(instr.r_type.rd, instr.full);
                else
                    vu0->ctc(instr.r_type.rd, regs[instr.r_type.rt].u32[0]);
                //printf("ctc2 %s, %d\n", Reg(instr.r_type.rt), instr.r_type.rd);
                break;
            case 0x10:
            case 0x11:
            case 0x12:
            case 0x13:
            case 0x14:
            case 0x15:
            case 0x16:
            case 0x17:
            case 0x18:
            case 0x19:
            case 0x1f:
                vu0->special1(instr);
                break;
            default:
                printf("[emu/CPU]: %s: Unknown cop2 instruction 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.r_type.rs);
                Application::Exit(1);
                break;
            }
            break;
        }
        case 0x14:
            beql(instr);
            break;
        case 0x15:
            bnel(instr);
            break;
        case 0x19:
            daddiu(instr);
            break;
        case 0x1a:
            ldl(instr);
            break;
        case 0x1b:
            ldr(instr);
            break;
        case 0x1c:
        {
            switch (instr.r_type.func)
            {
            case 0x12:
                mflo1(instr);
                break;
            case 0x18:
                mult1(instr);
                break;
            case 0x1b:
                divu1(instr);
                break;
            case 0x28:
                switch (instr.r_type.sa)
                {
                case 0x10:
                {
                    int reg1 = instr.r_type.rd;
                    int reg2 = instr.r_type.rt;
                    int dest = instr.r_type.rs;

                    for (int i = 0; i < 4; i++)
                    {
                        int64_t value = (int32_t)regs[reg1].u32[i];
                        value += (int32_t)regs[reg2].u32[i];
                        if (value > 0x7FFFFFFF)
                            value = 0x7FFFFFFF;
                        if (value < (int32_t)0x80000000)
                            value = (int32_t)0x80000000;
                        regs[dest].u32[i] = (int32_t)value;
                    }
                }
                break;
                default:
                    printf("[emu/CPU]: %s: Unknown mmi1 instruction 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.r_type.sa);
                    Application::Exit(1);
                    break;
                }
                break;
            case 0x29:
                switch (instr.r_type.sa)
                {
                case 0x12:
                    por(instr);
                    break;
                default:
                    printf("[emu/CPU]: %s: Unknown mmi3 instruction 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.r_type.sa);
                    Application::Exit(1);
                    break;
                }
                break;
            default:
                printf("[emu/CPU]: %s: Unknown mmi instruction 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.r_type.func);
                Application::Exit(1);
                break;
            }
        }
        break;
        case 0x1e:
            lq(instr);
            break;
        case 0x1f:
            sq(instr);
            break;
        case 0x20:
            lb(instr);
            break;
        case 0x21:
            lh(instr);
            break;
        case 0x23:
            lw(instr);
            break;
        case 0x24:
            lbu(instr);
            break;
        case 0x25:
            lhu(instr);
            break;
        case 0x27:
            lwu(instr);
            break;
        case 0x28:
            sb(instr);
            break;
        case 0x29:
            sh(instr);
            break;
        case 0x2B:
            sw(instr);
            break;
        case 0x2C:
            sdl(instr);
            break;
        case 0x2D:
            sdr(instr);
            break;
        case 0x2f:
            //printf("cache\n");
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
        
        cycles_to_execute--;
    }

    cop0_regs[9] += cycles + std::abs(cycles_to_execute);

    regs[0].u64[0] = regs[0].u64[1] = 0;

	if (bus->read<uint32_t>(0x1000f000) & bus->read<uint32_t>(0x1000f010))
	{
		printf("Need to trigger interrupt here\n");
		exit(1);
	}
}

void EmotionEngine::Dump()
{
    for (int i = 0; i < 32; i++)
        printf("[emu/CPU]: %s: %s\t->\t%s\n", __FUNCTION__, Reg(i), print_128(regs[i]));
    printf("[emu/CPU]: %s: pc\t->\t0x%08x\n", __FUNCTION__, pc-4);
    printf("[emu/CPU]: %s: hi\t->\t0x%08lx\n", __FUNCTION__, hi);
    printf("[emu/CPU]: %s: lo\t->\t0x%08lx\n", __FUNCTION__, lo);
    printf("[emu/CPU]: %s: hi1\t->\t0x%08lx\n", __FUNCTION__, hi1);
    printf("[emu/CPU]: %s: lo1\t->\t0x%08lx\n", __FUNCTION__, lo1);
}