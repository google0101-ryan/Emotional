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

    struct CacheTag
    {
        bool valid = false;
        bool dirty = false;
        bool lrf = false;
        uint32_t page = 0;
    };

    struct Cache
    {
        CacheTag tag[2];
        uint8_t data[2][64] = {0};
    } icache[128], dcache[64];

    bool isCacheEnabled = false;

    uint32_t Read32Instr(uint32_t addr)
    {
        if (!bus->IsCacheable(addr) || !isCacheEnabled)
            return bus->read<uint32_t>(addr);

        uint32_t page = (addr >> 14);
        uint32_t index = (addr >> 5) & 8;
        uint32_t offset = addr & 0x3F;

        Cache& line = icache[index];

        for (int way = 0; way < 2; way++)
        {
            if (line.tag[way].page == page && line.tag[way].valid)
            {
                return *((uint32_t*)&line.data[way][offset]);
            }
        }

        // Welp, cache miss
        printf("[emu/CPU]: Cache miss for address 0x%08x\n", addr);

        int replace = 0;
        if (!line.tag[0].valid && line.tag[1].valid)
            replace = 0;
        else if (line.tag[0].valid && !line.tag[1].valid)
            replace = 1;
        else
            replace = line.tag[0].lrf ^ line.tag[1].lrf;
        
        for (int i = 0; i < 64; i++)
            line.data[replace][i] = bus->read<uint8_t>((page << 14) + i);
        return *(uint32_t*)&line.data[replace][offset];
    }

    void Write32(uint32_t addr, uint32_t data)
    {
        if (!bus->IsCacheable(addr) || !isCacheEnabled)
        {
            bus->write(addr, data);
            return;
        }

        uint32_t page = (addr >> 13);
        uint32_t index = (addr >> 5) & 0b1111111;
        uint32_t offset = addr & 0x3F;

        Cache& line = dcache[index];

        for (int way = 0; way < 2; way++)
        {
            if (line.tag[way].page == page && line.tag[way].valid)
            {
                *((uint32_t*)&line.data[way][offset]) = data;
                line.tag[way].dirty = true; // We mark as dirty to flush back to main memory on eviction
            }
        }

        printf("[emu/CPU]: Cache miss for address 0x%08x\n", addr);

        int replace = 0;
        if (!line.tag[0].valid && line.tag[1].valid)
            replace = 0;
        else if (line.tag[0].valid && !line.tag[1].valid)
            replace = 1;
        else
            replace = line.tag[0].lrf ^ line.tag[1].lrf;

        if (line.tag[replace].dirty)
        {
            for (int i = 0; i < 64; i++)
                bus->write((page << 13) + i, line.data[replace][i]);
        }
        
        for (int i = 0; i < 64; i++)
        {
            line.data[replace][i] = bus->read<uint8_t>((page << 13) + i);
        }
        *(uint32_t*)&line.data[replace][offset] = data;
        line.tag[replace].dirty = true;
    }

    void Write64(uint32_t addr, uint64_t data)
    {
        if (!bus->IsCacheable(addr) || !isCacheEnabled)
        {
            bus->write(addr, data);
            return;
        }

        uint32_t page = (addr >> 13);
        uint32_t index = (addr >> 5) & 0b1111111;
        uint32_t offset = addr & 0x3F;

        Cache& line = dcache[index];

        for (int way = 0; way < 2; way++)
        {
            if (line.tag[way].page == page && line.tag[way].valid)
            {
                *((uint64_t*)&line.data[way][offset]) = data;
                line.tag[way].dirty = true; // We mark as dirty to flush back to main memory on eviction
            }
        }

        printf("[emu/CPU]: Cache miss for address 0x%08x\n", addr);

        int replace = 0;
        if (!line.tag[0].valid && line.tag[1].valid)
            replace = 0;
        else if (line.tag[0].valid && !line.tag[1].valid)
            replace = 1;
        else
            replace = line.tag[0].lrf ^ line.tag[1].lrf;

        if (line.tag[replace].dirty)
        {
            for (int i = 0; i < 64; i++)
                bus->write((page << 13) + i, line.data[replace][i]);
        }
        
        for (int i = 0; i < 64; i++)
        {
            line.data[replace][i] = bus->read<uint8_t>((page << 13) + i);
        }
        *(uint64_t*)&line.data[replace][offset] = data;
        line.tag[replace].dirty = true;
    }

    void j(Opcode i); // 0x03
    void beq(Opcode i); // 0x04
    void bne(Opcode i); // 0x05
    void addiu(Opcode i); // 0x09
    void slti(Opcode i); // 0x0A
    void andi(Opcode i); // 0x0C
    void ori(Opcode i); // 0x0D
    void lui(Opcode i); // 0x0F
    void mfc0(Opcode i); // 0x10 0x00
    void mtc0(Opcode i); // 0x10 0x04
    void sw(Opcode i); // 0x2B
    void sd(Opcode i); // 0x3f
    
    void sll(Opcode i); // 0x00
    void jr(Opcode i); // 0x08
    void jalr(Opcode i); // 0x09
    void mult(Opcode i); // 0x18
    void op_or(Opcode i); // 0x25
    void daddu(Opcode i); // 0x2d

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