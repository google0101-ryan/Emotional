#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <util/uint128.h>

union GIFTag
{
    uint64_t regs : 64,
    nregs : 4,
    format : 2,
    prim_en : 1,
    prim_data : 11,
    unused1 : 30,
    eop : 1,
    nloop : 15;
};

// Graphics InterFace, used for unpacking DMA transfers and uploading them
// to the GS
class GIF
{
private:
    int data_count = 0;
    int reg_count = 0;
    uint32_t gif_ctrl;
    std::queue<uint128_t> fifo;
    GIFTag tag;
public:
    void tick(int cycles);
    void process_packed(uint128_t qword);

    uint32_t read32(uint32_t addr)
    {
        switch (addr)
        {
        case 0x10003020:
            return (fifo.size() & 0x1F);
        default:
            printf("[emu/Gif]: %s: Read from unknown addr 0x%08x\n", __FUNCTION__, addr);
            exit(1);
        }
    }

    void write32(uint32_t addr, uint128_t data)
    {
        switch (addr)
        {
        case 0x10003000:
            gif_ctrl = data.u32[0];
            return;
        case 0x10006000:
        {
            fifo.push(data);
            return;
        }
        default:
            printf("[emu/Gif]: %s: Write to unknown addr 0x%08x\n", __FUNCTION__, addr);
            exit(1);
        }
    }
};