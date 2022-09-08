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

void EmotionEngine::jal(Opcode i)
{
    uint32_t target = (i.j_type.target << 2);
    regs[31].u64[0] = next_pc;
    next_pc = (pc & 0xF0000000) | target;
    printf("jal 0x%08x\n", next_pc);
}

void EmotionEngine::beq(Opcode i)
{
    int32_t off = (int32_t)((int16_t)(i.i_type.imm));
    int32_t imm = off << 2;

    printf("beq %s, %s, 0x%08x\n", Reg(i.i_type.rs), Reg(i.i_type.rt), next_pc + imm);

    if ((int32_t)regs[i.i_type.rs].u32[0] == (int32_t)regs[i.i_type.rt].u32[0])
    {
        next_pc = pc + imm;
    }   
}

void EmotionEngine::bne(Opcode i)
{
    int32_t off = (int32_t)((int16_t)(i.i_type.imm));
    int32_t imm = off << 2;


    printf("0x%08x, 0x%08x\n", regs[i.i_type.rs].u32[0], regs[i.i_type.rt].u32[0]);
    printf("bne %s, %s, 0x%08x\n", Reg(i.i_type.rs), Reg(i.i_type.rt), pc + imm);

    if ((int32_t)regs[i.i_type.rs].u32[0] != (int32_t)regs[i.i_type.rt].u32[0])
    {
        next_pc = pc + imm;
    }
}

void EmotionEngine::blez(Opcode i)
{
    int32_t off = (int32_t)((int16_t)(i.i_type.imm));
    int32_t imm = off << 2;

    int rs = i.r_type.rs;

    printf("blez %s, 0x%08x\n", Reg(rs), pc + imm);

    if ((int32_t)regs[rs].u32[0] <= 0)
    {
        next_pc = pc + imm;
    }
}

void EmotionEngine::bgtz(Opcode i)
{
    int32_t off = (int32_t)((int16_t)(i.i_type.imm));
    int32_t imm = off << 2;

    int rs = i.r_type.rs;

    printf("bgtz %s, 0x%08x\n", Reg(rs), pc + imm);

    if ((int32_t)regs[rs].u32[0] > 0)
    {
        next_pc = pc + imm;
    }
}

void EmotionEngine::addiu(Opcode i)
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    int16_t imm = (int16_t)i.i_type.imm;

    int32_t result = regs[rs].u64[0] + imm;
    regs[rt].u64[0] = result;

    printf("addiu %s, %s, %d\n", Reg(rt), Reg(rs), imm);
}

void EmotionEngine::slti(Opcode i)
{
    regs[i.i_type.rt].u64[0] = (int32_t)regs[i.i_type.rs].u32[0] < (int32_t)(uint32_t)i.i_type.imm;
    printf("slti %s, %s, 0x%04x\n", Reg(i.i_type.rt), Reg(i.i_type.rs), i.i_type.imm);
}

void EmotionEngine::sltiu(Opcode i)
{
    regs[i.i_type.rt].u64[0] = regs[i.i_type.rs].u32[0] < (uint32_t)i.i_type.imm;
    printf("sltiu %s, %s, 0x%04x\n", Reg(i.i_type.rt), Reg(i.i_type.rs), i.i_type.imm);
}

void EmotionEngine::andi(Opcode i)
{
    regs[i.i_type.rt].u64[0] = regs[i.i_type.rs].u32[0] & (uint32_t)(int32_t)i.i_type.imm;
    printf("andi %s, %s, 0x%08x\n", Reg(i.i_type.rt), Reg(i.i_type.rs), i.i_type.imm);
}

void EmotionEngine::ori(Opcode i)
{
    regs[i.i_type.rt].u64[0] = regs[i.i_type.rs].u32[0] | (uint32_t)(int32_t)i.i_type.imm;
    printf("ori %s, %s, 0x%08x\n", Reg(i.i_type.rt), Reg(i.i_type.rs), i.i_type.imm);
}

void EmotionEngine::lui(Opcode i)
{
    regs[i.i_type.rt].u64[0] = ((uint32_t)i.i_type.imm << 16);
    printf("lui %s, 0x%08x\n", Reg(i.i_type.rt), regs[i.i_type.rt].u32[0]);
}

void EmotionEngine::mfc0(Opcode i)
{
    regs[i.r_type.rt].u64[0] = cop0_regs[i.r_type.rd];
    printf("mfc0 %s, r%d\n", Reg(i.r_type.rt), i.r_type.rd);
}

void EmotionEngine::mtc0(Opcode i)
{
    cop0_regs[i.r_type.rd] = regs[i.r_type.rt].u32[0];
    printf("mtc0 %s, r%d\n", Reg(i.r_type.rt), i.r_type.rd);
}

void EmotionEngine::beql(Opcode i)
{
    int32_t off = (int32_t)((int16_t)(i.i_type.imm));
    int32_t imm = off << 2;

    printf("beql %s, %s, 0x%08x\n", Reg(i.i_type.rs), Reg(i.i_type.rt), next_pc + imm);

    if ((int32_t)regs[i.i_type.rs].u32[0] == (int32_t)regs[i.i_type.rt].u32[0])
    {
        next_pc = pc + imm;
    }
    else
        AdvancePC(); // Skip delay slot
}

void EmotionEngine::bnel(Opcode i)
{
    int32_t off = (int32_t)((int16_t)(i.i_type.imm));
    int32_t imm = off << 2;

    bool taken = false;

    if ((int32_t)regs[i.i_type.rs].u32[0] != (int32_t)regs[i.i_type.rt].u32[0])
    {
        next_pc = pc + imm;
        taken = true;
    }
    else
        AdvancePC(); // Skip delay slot
    
    printf("bnel %s, %s, 0x%08x (%s)\n", Reg(i.i_type.rs), Reg(i.i_type.rt), pc + imm, taken ? "taken" : "");
}

void EmotionEngine::lb(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    regs[rt].u64[0] = (int64_t)((uint8_t)(Read32(addr) & 0xFF));

    printf("lb %s, %d(%s(0x%x))\n", Reg(rt), off, Reg(base), regs[rt]);
}

void EmotionEngine::lw(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    regs[rt].u64[0] = bus->read<uint32_t>(addr);

    printf("lw %s, %d(%s(0x%x))\n", Reg(rt), off, Reg(base), regs[base].u32[0]);
}

void EmotionEngine::lbu(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    regs[rt].u64[0] = (Read32(addr) & 0xFF);

    printf("lbu %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void EmotionEngine::sb(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    bus->write<uint8_t>(addr, regs[rt].u32[0] & 0xFF);

    printf("sb %s, %d(%s)\n", Reg(rt), off, Reg(base));
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

void EmotionEngine::ld(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    regs[rt].u64[0] = bus->read<uint64_t>(addr);

    printf("ld %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void EmotionEngine::swc1(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    Write32(addr, cop1.i[rt]);

    printf("swc1 f%d, %d(%s)\n", rt, off, Reg(base));
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

    regs[rd].u64[0] = (uint64_t)(int32_t)(regs[rt].u32[0] << sa);

    printf("sll %s, %s, %d\n", Reg(rd), Reg(rt), sa);
}

void EmotionEngine::sra(Opcode i)
{
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;
    int sa = i.r_type.sa;

    int32_t reg = (int32_t)regs[rt].u32[0];
    regs[rd].u64[0] = reg >> sa;

    printf("sra %s, %s, %d\n", Reg(rd), Reg(rt), sa);
}

void EmotionEngine::jalr(Opcode i)
{
    int rs = i.r_type.rs;
    int rd = i.r_type.rd;
    
    regs[rd].u64[0] = next_pc;
    next_pc = regs[rs].u32[0];

    printf("jalr %s, %s\n", Reg(rs), Reg(rd));
}

void EmotionEngine::movn(Opcode i)
{
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;

    if (regs[rt].u64[0] != 0) regs[rd].u64[0] = regs[rs].u64[0];

    printf("movn %s, %s, %s\n", Reg(rt), Reg(rd), Reg(rs));
}

void EmotionEngine::mfhi(Opcode i)
{
    int rd = i.r_type.rd;

    regs[rd].u64[0] = hi;

    printf("mfhi %s\n", Reg(rd));
}

void EmotionEngine::mflo(Opcode i)
{
    int rd = i.r_type.rd;

    regs[rd].u64[0] = lo;

    printf("mflo %s\n", Reg(rd));
}

void EmotionEngine::mult(Opcode i)
{
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;

    int64_t reg1 = (int64_t)regs[rs].u32[0];
    int64_t reg2 = (int64_t)regs[rt].u32[0];
    int64_t result = reg1 * reg2;

    regs[rd].u64[0] = lo = (int32_t)(result & 0xFFFFFFFF);
    hi = (int32_t)(result >> 32);

    printf("mult %s, %s, %s (0x%08x, 0x%08x)\n", Reg(rd), Reg(rs), Reg(rt), hi, lo);
}

void EmotionEngine::div(Opcode i)
{
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;
    
    int32_t reg1 = regs[rs].u32[0];
    int32_t reg2 = regs[rt].u32[0];

    if (reg2 == 0)
    {
        hi = reg1;
        lo = reg1 >= 0 ? (int32_t)0xffffffff : 1;
    }
    else if (reg1 == 0x80000000 && reg2 == -1)
    {
        hi = 0;
        lo = (int32_t)0x80000000;
    }
    else
    {
        hi = (int32_t)(reg1 % reg2);
        lo = (int32_t)(reg1 / reg2);
    }

    printf("div %s, %s\n", Reg(rs), Reg(rt));
}

void EmotionEngine::divu(Opcode i)
{
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;

    if (regs[rt].u32[0] == 0)
    {
        hi = (int32_t)regs[rs].u32[0];
        lo = (int32_t)0xffffffff;
    }
    else
    {
        hi = (int32_t)(regs[rs].u32[0] % regs[rt].u32[0]);
        lo = (int32_t)(regs[rs].u32[0] / regs[rt].u32[0]);
    }

    printf("divu %s (0x%08x), %s (0x%08x) (0x%08x, 0x%08x)\n", Reg(rs), regs[rs].u32[0], Reg(rt), regs[rt].u32[0], hi, lo);
}

void EmotionEngine::addu(Opcode i)
{
    int rt = i.r_type.rt;
    int rs = i.r_type.rs;
    int rd = i.r_type.rd;

    int32_t result = regs[rs].u64[0] + regs[rt].u64[0];
    regs[rd].u64[0] = result;

    printf("addu %s, %s, %s\n", Reg(rd), Reg(rs), Reg(rt));
}

void EmotionEngine::subu(Opcode i)
{
    int rt = i.r_type.rt;
    int rs = i.r_type.rs;
    int rd = i.r_type.rd;

    int32_t reg1 = regs[rs].u64[0];
    int32_t reg2 = regs[rt].u64[0];
    regs[rd].u64[0] = reg1 - reg2;

    printf("subu %s, %s, %s\n", Reg(rd), Reg(rs), Reg(rt));
}

void EmotionEngine::op_or(Opcode i)
{
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;

    regs[rd].u64[0] = regs[rt].u32[0] | regs[rs].u32[0];

    printf("or %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void EmotionEngine::sltu(Opcode i)
{
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;

    regs[rd].u64[0] = regs[rs].u64[0] < regs[rt].u64[0];

    printf("sltu %s, %s, %s\n", Reg(rd), Reg(rs), Reg(rt));
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

void EmotionEngine::bltz(Opcode i)
{
    int rs = i.i_type.rs;

    int32_t val = (int32_t)regs[rs].u32[0];
    int32_t off = (int32_t)((int16_t)(i.i_type.imm));
    int32_t imm = off << 2;

    printf("bltz %s, 0x%08x\n", Reg(rs), pc + off);

    if (val < 0)
    {
        next_pc = pc + imm;
    }
}

void EmotionEngine::bgez(Opcode i)
{
    int rs = i.i_type.rs;

    int32_t val = (int32_t)regs[rs].u32[0];
    int32_t off = (int32_t)((int16_t)(i.i_type.imm));
    int32_t imm = off << 2;

    printf("bgez %s, 0x%08x\n", Reg(rs), pc + off);

    if (val >= 0)
    {
        next_pc = pc + imm;
    }
}