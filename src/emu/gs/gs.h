#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>

class GraphicsSynthesizer
{
private:
    uint64_t csr; // Control & Status Register, unknown what the layout is
	uint64_t imr; // Interrupt control register
	uint64_t pmode; // Misc. mode settings
public:
	uint64_t read32(uint32_t addr)
	{
		switch (addr)
		{
		case 0x12001000:
			return csr;
		default:
            printf("[emu/GS]: %s: Read from unknown addr 0x%08x\n", __FUNCTION__, addr);
            exit(1);
		}
	}

    void write32(uint32_t addr, uint64_t data)
    {
        switch (addr)
        {
		case 0x12000000:
			pmode = data;
			return;
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
        case 0x12001010:
			imr = data;
			return;
        default:
            printf("[emu/GS]: %s: Write to unknown addr 0x%08x\n", __FUNCTION__, addr);
            exit(1);
        }
    }
};