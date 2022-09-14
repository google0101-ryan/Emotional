#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>

class GraphicsSynthesizer
{
private:
    uint32_t csr; // Control & Status Register, unknown what the layout is
public:
    void write32(uint32_t addr, uint32_t data)
    {
        switch (addr)
        {
        case 0x12000010:
        case 0x12000020:
        case 0x12000030:
        case 0x12000040:
        case 0x12000050:
        case 0x12000060:
            return;
        case 0x12001000:
            csr = data;
            return;
        default:
            printf("[emu/GS]: %s: Write to unknown addr 0x%08x\n", __FUNCTION__, addr);
            exit(1);
        }
    }
};