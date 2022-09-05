#pragma once

#include <bits/stdint-uintn.h>
#include <string>

class Bus
{
private:
    uint8_t ram[0x2000000];
    uint8_t bios[0x400000];
    uint8_t scratchpad[0x4000];

    uint32_t Translate(uint32_t addr)
    {
        if (addr <= 0x7FFFFFFF)
            return addr;
        else if (addr >= 0x80000000 && addr <= 0x9FFFFFFF)
            return addr - 0x80000000;
        else if (addr >= 0xA0000000 && addr <= 0xBFFFFFFF)
            return addr - 0xA0000000;
        else if (addr >= 0xC0000000 && addr <= 0xDFFFFFFF)
            return addr - 0xC0000000;
        else if (addr >= 0xE0000000 && addr <= 0xFFFFFFF)
            return addr - 0xE0000000;
        else
            return 0xFFFFFFFF;
    }
public:
    Bus(std::string biosName, bool& success);

    template<typename T>
    T read(uint32_t addr)
    {
        addr = Translate(addr);

        if (addr >= 0x1FC00000 && addr < 0x21C00000)
            return *(T*)&bios[addr - 0x1FC00000];
        
        printf("[emu/Bus]: %s: Failed to read from address 0x%08x\n", __FUNCTION__, addr);
        exit(1);
    }

    template<typename T>
    void write(uint32_t addr, T data)
    {
        addr = Translate(addr);

        if (addr >= 0x70000000 && addr < 0x70004000)
        {
            *(T*)&scratchpad[addr - 0x70000000] = data;
            return;
        }

        switch (addr)
        {
        case 0x1000f500:
            return;
        }

        printf("[emu/Bus]: %s: Write to unknown addr 0x%08x\n", __FUNCTION__, addr);
        exit(1);
    }

    // Returns whether a region of memory is cacheable by the bus
    bool IsCacheable(uint32_t addr)
    {
        if (addr <= 0x7FFFFFFF)
            return true;
        else if (addr >= 0x80000000 && addr <= 0x9FFFFFFF)
            return true;
        else if (addr >= 0xA0000000 && addr <= 0xBFFFFFFF)
            return false;
        else if (addr >= 0xC0000000 && addr <= 0xDFFFFFFF)
            return true;
        else if (addr >= 0xE0000000 && addr <= 0xFFFFFFF)
            return true;
        else
            return false;
    }
};