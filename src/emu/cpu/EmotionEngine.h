#pragma once

#include "emu/Bus.h"
#include "util/uint128.h"
#include <bits/stdint-uintn.h>


class EmotionEngine
{
private:
    Bus* bus;
    uint128_t regs[32];
    uint32_t pc, next_pc;
    uint64_t hi, lo;

    struct CacheTag
    {
        bool valid = false;
        bool dirty = false;
        uint32_t page = 0;
    };

    struct Cache
    {
        CacheTag tag;
        uint8_t data[64] = {0};
    } icache[128], dcache[64];

    uint32_t Read32Instr(uint32_t addr)
    {
        uint32_t page = (addr >> 14);
        uint32_t index = (addr >> 5) & 8;
        uint32_t offset = addr & 0x3F;

        Cache& line = icache[index];

        if (line.tag.page != page || !line.tag.valid)
        {
            printf("[emu/CPU]: Cache miss at 0x%08x\n", addr);
            if (line.tag.dirty)
            {
                uint32_t p = (line.tag.page << 14);
                for (int i = 0; i < 64; i++)
                    bus->write(p+i, line.data[i]);
            }
            line.tag.page = page;
            line.tag.valid = true;
            line.tag.dirty = false;
            for (int i = 0; i < 64; i++)
            {
                line.data[i] = bus->read<uint8_t>((page << 14) + i);
            }
            return *(uint32_t*)&line.data[offset];
        }
        else 
        {
            printf("Cache hit at 0x%08x\n", addr);
            return *(uint32_t*)&line.data[offset];
        }
    }

    void AdvancePC()
    {
        pc = next_pc;
        next_pc += 4;
    }
public:
    EmotionEngine(Bus* bus);

    void Clock();
    void Dump();
};