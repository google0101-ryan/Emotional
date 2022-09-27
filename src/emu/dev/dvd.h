#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>

class CDVD
{
private:
    uint8_t n_status = 0x40;
public:
    uint32_t read(uint32_t addr)
    {
        switch (addr)
        {
        case 0x1f402005:
            return n_status;
        default:
            printf("[emu/CDVD]: %s: Read from unknown address 0x%08x\n", __FUNCTION__, addr);
            exit(1);
        }
    }
};