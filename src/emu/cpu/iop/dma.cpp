#include <emu/cpu/iop/dma.h>
#include <app/Application.h>
#include <emu/iopbus.h>
#include <emu/Bus.h>

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
        for (int id = 7; id < 13; id++)
		{
			auto& channel = channels[id];
			bool enable = globals.dpcr2 & (1 << ((id - 7) * 4 + 3));
			if (channel.control.running && enable)
			{
				switch (id)
				{
				case DMAChannels::SIF1:
				{
					auto sif = bus->GetSif();
					if (!sif->fifo1.empty())
					{
						auto data = sif->fifo1.front();
						sif->fifo1.pop();

						bus->write<uint32_t>(channel.address, data);
						channel.address += 4;
						channel.block_conf.count--;
					}
					break;
				}
				default:
					printf("Transfer on unknown channel %d\n", id);
					exit(1);
				}
			}
		}
    }
}