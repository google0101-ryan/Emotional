#pragma once

#include "emu/Bus.h"
#include "emu/cpu/opcode.h"
#include "util/uint128.h"
#include <bits/stdint-uintn.h>
#include <emu/cpu/ee/vu.h>

class EmotionEngine
{
private:
    Bus* bus;
    VectorUnit* vu0;

    uint128_t regs[32];
    uint32_t cop0_regs[32];
    uint32_t pc;
    uint64_t hi, lo;
    uint64_t hi1, lo1;

    Opcode next_instr, instr;

    void fetch_next()
    {
        next_instr = {};
        next_instr.full = bus->read<uint32_t>(pc);
        next_instr.pc = pc;
        pc += 4;
    }

    struct COP1
    {
        union
        {
            float f[32] = {0.0f};
            uint32_t i[32];
        } regs;

        union
        {
            float f;
            uint32_t u;
            int32_t i;
        } accumulator;
    } cop1;

    struct LoadDelaySlot
    {
        uint32_t reg;
        uint128_t value;
    } cur_delay, next_delay;

    void HandleLoadDelay()
    {
        regs[cur_delay.reg] = cur_delay.value;
        cur_delay = next_delay;
        next_delay.reg = 0;
        next_delay.value.u64[0] = next_delay.value.u64[1] = 0;
    }

    struct CacheTag
    {
        bool valid = false;
        bool dirty = false;
        bool lrf = false;
        uint32_t page = 0;
    };

    struct ICacheLine
    {
        bool lfu[2];
        uint32_t tag[2];
        uint8_t data[2][64];
    };

    ICacheLine icache[128];

    struct DCacheLine
    {
        bool lfu[2];
        bool dirty[2];
        uint32_t tag[2];
        uint8_t data[2][64];
    };

    DCacheLine dcache[64];

    bool isCacheEnabled = false;

    bool singleStep = false;

    void j(Opcode i); // 0x02
    void jal(Opcode i); // 0x03
    void beq(Opcode i); // 0x04
    void bne(Opcode i); // 0x05
    void blez(Opcode i); // 0x06
    void bgtz(Opcode i); // 0x07
    void addiu(Opcode i); // 0x09
    void slti(Opcode i); // 0x0A
    void sltiu(Opcode i); // 0x0B
    void andi(Opcode i); // 0x0C
    void ori(Opcode i); // 0x0D
    void xori(Opcode i); // 0x0E
    void lui(Opcode i); // 0x0F
    void mfc0(Opcode i); // 0x10 0x00
    void mtc0(Opcode i); // 0x10 0x04
    void beql(Opcode i); // 0x14
    void bnel(Opcode i); // 0x15
    void daddiu(Opcode i); // 0x19
    void ldl(Opcode i); // 0x1A
    void ldr(Opcode i); // 0x1B
    void lq(Opcode i); // 0x1E
    void sq(Opcode i); // 0x1F
    void lb(Opcode i); // 0x20
    void lh(Opcode i); // 0x21
    void lw(Opcode i); // 0x23
    void lbu(Opcode i); // 0x24
    void lhu(Opcode i); // 0x25
    void lwu(Opcode i); // 0x27
    void sb(Opcode i); // 0x28
    void sh(Opcode i); // 0x29
    void sw(Opcode i); // 0x2B
    void sdl(Opcode i); // 0x2C
    void sdr(Opcode i); // 0x2D
    void ld(Opcode i); // 0x37
    void swc1(Opcode i); // 0x39
    void sd(Opcode i); // 0x3f
    
    void sll(Opcode i); // 0x00
    void srl(Opcode i); // 0x02
    void sra(Opcode i); // 0x03
    void sllv(Opcode i); // 0x04
    void srlv(Opcode i); // 0x06
    void srav(Opcode i); // 0x07
    void jr(Opcode i); // 0x08
    void jalr(Opcode i); // 0x09
    void movz(Opcode i); // 0x0a
    void movn(Opcode i); // 0x0B
    void mfhi(Opcode i); // 0x10
    void mflo(Opcode i); // 0x12
    void dsllv(Opcode i); // 0x14
    void dsrav(Opcode i); // 0x17
    void mult(Opcode i); // 0x18
    void div(Opcode i); // 0x1A
    void divu(Opcode i); // 0x1B
    void addu(Opcode i); // 0x21
    void subu(Opcode i); // 0x23
    void op_and(Opcode i); // 0x24
    void op_or(Opcode i); // 0x25
    void op_nor(Opcode i); // 0x27
    void slt(Opcode i); // 0x2a
    void sltu(Opcode i); // 0x2b
    void daddu(Opcode i); // 0x2d
    void dsll(Opcode i); // 0x38
    void dsrl(Opcode i); // 0x3a
    void dsll32(Opcode i); // 0x3c
    void dsrl32(Opcode i); // 0x3e
    void dsra32(Opcode i); // 0x3f

    // RegImm

    void bltz(Opcode i); // 0x00
    void bgez(Opcode i); // 0x01

    // mmi

    void mflo1(Opcode i); // 0x12
    void mult1(Opcode i); // 0x18
    void divu1(Opcode i); // 0x1b

    // mmi3

    void por(Opcode i); // 0x12

    // cop1

    void mfc1(Opcode i);
    void mtc1(Opcode i);

    // cop1.w
    void adda(Opcode i);

    const char* Reg(int index)
    {
        switch (index)
        {
        case 0:
            return "$zero";
        case 1:
            return "$at";
        case 2:
            return "$v0";
        case 3:
            return "$v1";
        case 4:
            return "$a0";
        case 5:
            return "$a1";
        case 6:
            return "$a2";
        case 7:
            return "$a3";
        case 8:
            return "$t0";
        case 9:
            return "$t1";
        case 10:
            return "$t2";
        case 11:
            return "$t3";
        case 12:
            return "$t4";
        case 13:
            return "$t5";
        case 14:
            return "$t6";
        case 15:
            return "$t7";
        case 16:
            return "$s0";
        case 17:
            return "$s1";
        case 18:
            return "$s2";
        case 19:
            return "$s3";
        case 20:
            return "$s4";
        case 21:
            return "$s5";
        case 22:
            return "$s6";
        case 23:
            return "$s7";
        case 24:
            return "$t8";
        case 25:
            return "$t9";
        case 26:
            return "$k0";
        case 27:
            return "$k1";
        case 28:
            return "$gp";
        case 29:
            return "$sp";
        case 30:
            return "$fp";
        case 31:
            return "$ra";
        default:
            return "";
        }
    }

	bool branch_taken = false;
public:
    EmotionEngine(Bus* bus, VectorUnit* vu0);

    void Clock(int cycles);
    void Dump();
};