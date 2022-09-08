#pragma once

#include "emu/Bus.h"
#include "emu/cpu/opcode.h"
#include "util/uint128.h"
#include <bits/stdint-uintn.h>


class EmotionEngine
{
private:
    Bus* bus;
    uint128_t regs[32];
    uint32_t cop0_regs[32];
    uint32_t pc, next_pc;
    uint64_t hi, lo;

    struct COP1
    {
        union
        {
            float f[32] = {0.0f};
            uint32_t i[32];
        };
    } cop1;

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

    bool isCacheEnabled = false;

    uint32_t Read32(uint32_t addr, bool isInstr = false)
    {
        if (!bus->IsCacheable(addr) || !isCacheEnabled || !isInstr)
            return bus->read<uint32_t>(addr);

        int index = (addr >> 6) & 0x7F;
        uint16_t tag = addr >> 13;
        int off = addr & 0x3F;

        ICacheLine& line = icache[index];

        if (line.tag[0] != tag)
        {
            if (line.tag[1] != tag)
            {
                printf("Cache miss for addr 0x%08x\n", addr);
                if (line.tag[0] & (1 << 31))
                {
                    line.lfu[0] ^= true;
                    line.tag[0] = tag;
                    for (int i = 0; i < 64; i++)
                        line.data[0][i] = bus->read<uint8_t>((addr & 0xFFFFFFC0) + i);
                    return *(uint32_t*)&line.data[0][off];
                }
                else if (line.tag[1] & (1 << 31))
                {
                    line.lfu[1] ^= true;
                    line.tag[1] = tag;
                    for (int i = 0; i < 64; i++)
                        line.data[1][i] = bus->read<uint8_t>((addr & 0xFFFFFFC0) + i);
                    return *(uint32_t*)&line.data[1][off];
                }
                else
                {
                    int replace = line.lfu[0] ^ line.lfu[1];
                    line.lfu[replace] ^= true;
                    line.lfu[replace] = tag;
                    for (int i = 0; i < 64; i++)
                        line.data[replace][i] = bus->read<uint8_t>((addr & 0xFFFFFFC0) + i);
                    return *(uint32_t*)&line.data[replace][off];
                }
            }
            else
            {
                return *(uint32_t*)&line.data[1][off];
            }
        }
        else
        {
            return *(uint32_t*)&line.data[0][off];
        }
    }

    void Write32(uint32_t addr, uint32_t data)
    {
        if (!bus->IsCacheable(addr) || !isCacheEnabled)
        {
            bus->write(addr, data);
            return;
        }

        
    }

    void Write64(uint32_t addr, uint64_t data)
    {
        if (!bus->IsCacheable(addr) || !isCacheEnabled)
        {
            bus->write(addr, data);
            return;
        }
    }

    void j(Opcode i); // 0x02
    void jal(Opcode i); // 0x03
    void beq(Opcode i); // 0x04
    void bne(Opcode i); // 0x05
    void addiu(Opcode i); // 0x09
    void slti(Opcode i); // 0x0A
    void sltiu(Opcode i); // 0x0B
    void andi(Opcode i); // 0x0C
    void ori(Opcode i); // 0x0D
    void lui(Opcode i); // 0x0F
    void mfc0(Opcode i); // 0x10 0x00
    void mtc0(Opcode i); // 0x10 0x04
    void beql(Opcode i); // 0x14
    void bnel(Opcode i); // 0x15
    void lb(Opcode i); // 0x20
    void lw(Opcode i); // 0x23
    void lbu(Opcode i); // 0x24
    void sb(Opcode i); // 0x28
    void sw(Opcode i); // 0x2B
    void ld(Opcode i); // 0x37
    void swc1(Opcode i); // 0x39
    void sd(Opcode i); // 0x3f
    
    void sll(Opcode i); // 0x00
    void sra(Opcode i); // 0x03
    void jr(Opcode i); // 0x08
    void jalr(Opcode i); // 0x09
    void mflo(Opcode i); // 0x12
    void mult(Opcode i); // 0x18
    void divu(Opcode i); // 0x1B
    void op_or(Opcode i); // 0x25
    void daddu(Opcode i); // 0x2d

    void bltz(Opcode i); // 0x00

    void AdvancePC()
    {
        pc = next_pc;
        next_pc += 4;
    }

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
public:
    EmotionEngine(Bus* bus);

    void Clock();
    void Dump();
};