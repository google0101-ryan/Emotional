#include <emu/cpu/iop/dma.h>
#include <app/Application.h>
#include <emu/iopbus.h>

int GetChannelFromAddr(uint8_t addr)
{
    switch (addr)
    {
    case 0x00:
        return 7;
    case 0x10:
        return 8;
    case 0x20:
        return 9;
    case 0x30:
        return 10;
    case 0x40:
        return 11;
    case 0x50:
        return 12;
    case 0x80:
        return 0;
    case 0x90:
        return 1;
    case 0xA0:
        return 2;
    case 0xB0:
        return 3;
    case 0xC0:
        return 4;
    case 0xD0:
        return 5;
    case 0xE0:
        return 6;
    }
    return 0;
}

constexpr const char* REGS[] =
{
	"Dn_MADR", "Dn_BCR",
	"Dn_CHCR", "Dn_TADR"
};

constexpr const char* GLOBALS[] =
{
	"DPCR", "DICR", "DPCR2",
	"DICR2", "DMACEN", "DMACINTEN"
};


void IoDma::write(uint32_t addr, uint32_t data)
{
    uint16_t group = (addr >> 8) & 1;

    if ((addr & 0x70) == 0x70)
    {
        uint16_t offset = ((addr & 0xf) >> 2) + 2 * group;
        auto ptr = (uint32_t*)&globals + offset;
        //printf("[emu/IoDma]: Writing 0x%08x to %s\n", data, GLOBALS[offset]);

        if (offset == 1)
        {
            auto& irq = globals.dicr;
            auto flags = irq.flags;

            irq.value = data;
            irq.flags = flags & ~((data >> 24) & 0x7f);
            irq.master_flag = irq.force || (irq.master_enable && ((irq.enable & irq.flags) > 0));
        }
        else if (offset == 3)
        {
            auto& irq = globals.dicr2;
            auto flags = irq.flags;

            irq.value = data;
            irq.flags = flags & ~((data >> 24) & 0x7f);
        }
        else
        {
            *ptr = data;
        }
    }
    else
    {
        uint16_t channel = ((addr & 0x70) >> 4) + group * 7;
        uint16_t offset = (addr & 0xf) >> 2;
        auto ptr = (uint32_t*)&channels[channel] + offset;

        printf("[emu/IoDma]: Writing 0x%08x to %s on channel %d\n", data, REGS[offset], channel);
        *ptr = data;

        if (channels[channel].control.running)
        {
            printf("[emu/IoDma]: Started transfer on channel %d\n", channel);
        }
    }
}

uint32_t IoDma::read(uint32_t addr)
{
    uint16_t group = (addr >> 8) & 1;

    if ((addr & 0x70) == 0x70)
    {
        uint16_t offset = ((addr & 0xf) >> 2) + 2 * group;
        auto ptr = (uint32_t*)&globals + offset;
        //printf("[emu/IoDma]: Reading from %s in global registers\n", GLOBALS[offset]);
        return *ptr;
    }
    else
    {
        uint16_t channel = ((addr & 0x70) >> 4) + group * 7;
		uint16_t offset = (addr & 0xf) >> 2;
        auto ptr = (uint32_t*)&channels[channel] + offset;
        printf("[emu/IoDma]: Reading from %s in channel %d\n", REGS[offset], channel);
        return *ptr;
    }
}

void IoDma::tick(int cycles)
{
    for (int cycle = cycles; cycle > 0; cycle--)
    {
        
    }
}