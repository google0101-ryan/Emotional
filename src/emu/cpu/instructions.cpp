#include "emu/cpu/opcode.h"
#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <cstdio>
#include <emu/cpu/EmotionEngine.h>

void EmotionEngine::j(Opcode i)
{
    uint32_t target = (i.j_type.target << 2);
    next_pc = (pc & 0xF0000000) | target;
    printf("j 0x%08x\n", next_pc);
}

void EmotionEngine::bne(Opcode i)
{
    int32_t imm = (int32_t)((uint32_t)i.i_type.imm << 2);

    printf("bne %s, %s, 0x%08x\n", Reg(i.i_type.rs), Reg(i.i_type.rt), next_pc + imm);

    if ((int32_t)regs[i.i_type.rs].u32[0] != (int32_t)regs[i.i_type.rt].u32[0])
    {
        next_pc = pc + imm;
    }
}

void EmotionEngine::addiu(Opcode i)
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    int32_t imm = (int16_t)(uint32_t)i.i_type.imm;

    regs[rt].u32[0] = (uint32_t)((int32_t)regs[rt].u32[0] + imm);

    printf("addiu %s, %s, %d\n", Reg(rt), Reg(rs), imm);
}

void EmotionEngine::slti(Opcode i)
{
    regs[i.i_type.rt].u32[0] = (int32_t)regs[i.i_type.rs].u32[0] < (int32_t)(uint32_t)i.i_type.imm;
    printf("slti %s, %s, 0x%04x\n", Reg(i.i_type.rt), Reg(i.i_type.rs), i.i_type.imm);
}

void EmotionEngine::andi(Opcode i)
{
    regs[i.i_type.rt].u32[0] = regs[i.i_type.rs].u32[0] & (uint32_t)(int32_t)i.i_type.imm;
    printf("andi %s, %s, 0x%08x\n", Reg(i.i_type.rt), Reg(i.i_type.rs), i.i_type.imm);
}

void EmotionEngine::ori(Opcode i)
{
    regs[i.i_type.rt].u32[0] = regs[i.i_type.rs].u32[0] | (uint32_t)(int32_t)i.i_type.imm;
    printf("ori %s, %s, 0x%08x\n", Reg(i.i_type.rt), Reg(i.i_type.rs), i.i_type.imm);
}

void EmotionEngine::lui(Opcode i)
{
    regs[i.i_type.rt].u32[0] = (int32_t)((uint32_t)i.i_type.imm << 16);
    printf("lui %s, 0x%08x\n", Reg(i.i_type.rt), regs[i.i_type.rt].u32[0]);
}

void EmotionEngine::mfc0(Opcode i)
{
    regs[i.r_type.rt].u32[0] = cop0_regs[i.r_type.rd];
    printf("mfc0 %s, r%d\n", Reg(i.r_type.rt), i.r_type.rd);
}

void EmotionEngine::mtc0(Opcode i)
{
    cop0_regs[i.r_type.rd] = regs[i.r_type.rt].u32[0];
    printf("mtc0 %s, r%d\n", Reg(i.r_type.rt), i.r_type.rd);
}

void EmotionEngine::sw(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    Write32(addr, regs[rt].u32[0]);

    printf("sw %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void EmotionEngine::sd(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    Write64(addr, regs[rt].u32[0]);

    printf("sd %s, %d(%s)\n", Reg(rt), off, Reg(base));
}


void EmotionEngine::jr(Opcode i)
{
    next_pc = regs[i.r_type.rs].u32[0];
    printf("jr %s\n", Reg(i.r_type.rs));
}


void EmotionEngine::sll(Opcode i)
{
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;
    int sa = i.r_type.sa;

    regs[rd].u32[0] = regs[rt].u32[0] << sa;

    printf("sll %s, %s, %d\n", Reg(rd), Reg(rt), sa);
}

void EmotionEngine::jalr(Opcode i)
{
    int rs = i.r_type.rs;
    int rd = i.r_type.rd;
    
    regs[rd].u32[0] = next_pc;
    next_pc = regs[rs].u32[0];

    printf("jalr %s, %s\n", Reg(rs), Reg(rd));
}

void EmotionEngine::daddu(Opcode i)
{
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;

    int64_t reg1 = regs[rs].u64[0];
    int64_t reg2 = regs[rt].u64[0];

    regs[rd].u64[0] = reg1 + reg2;

    printf("daddu %s, %s, %s\n", Reg(rd), Reg(rs), Reg(rt));
}