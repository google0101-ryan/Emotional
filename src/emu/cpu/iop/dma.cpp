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

void IoDma::write(uint32_t addr, uint32_t data)
{
    if ((addr >= 0x1F801080 && addr <= 0x1F8010EC) || (addr >= 0x1f801500 && addr <= 0x1f80155C))
    {
        int channel = GetChannelFromAddr(addr & 0xF0);

        printf("Channel %d -> %d\n", channel, addr & 0xF);

        switch (addr & 0xF)
        {
        case 0:
            channels[channel].madr = data;
            return;
        case 4:
            channels[channel].bcr = data;
            return;
        case 8:
            channels[channel].chcr = data;
            return;
        case 0xC:
            channels[channel].tadr = data;
            return;
        }
    }

    switch (addr)
    {
    case 0x1f8010f0:
        dpcr.value = data;
        break;
    case 0x1f8010f4:
        dicr.value = data;
        break;
    case 0x1f801570:
        dpcr2.value = data;
        break;
    case 0x1f801574:
        dicr2.value = data;
        break;
    case 0x1f801578:
        glob_enable = data;
        break;
    default:
        printf("[emu/IopDma]: %s: Write to unknown addr 0x%08x\n", __FUNCTION__, addr);
        exit(1);
    }
}

uint32_t IoDma::read(uint32_t addr)
{
    switch (addr)
    {
    case 0x1f8010f0:
        return dpcr.value;
    case 0x1f8010f4:
        return dicr.value;
    case 0x1f801570:
        return dpcr2.value;
    case 0x1f801574:
        return dicr2.value;
    case 0x1f801578:
        return glob_enable;
    default:
        printf("[emu/IopDma]: %s: Write to unknown addr 0x%08x\n", __FUNCTION__, addr);
        exit(1);
    }
}

void IoDma::tick(int cycles)
{
    for (int cycle = cycles; cycle > 0; cycle--)
    {
        for (int c = 0; c < 13; c++)
        {
            if (channels[c].chcr & (1 << 24) && glob_enable)
            {
                printf("Channel %d running\n", c);
                if (channels[c].bcr > 0)
                {
                    if (c == 10)
                    {    
                        auto sif = bus->GetSif();
                        if (sif->fifo1.size())
                        {
                            printf("Transfering word\n");
                            auto data = sif->fifo1.front();
                            sif->fifo1.pop();
                            bus->write<uint32_t>(channels[c].madr, data);
                            channels[c].madr += 4;
                            channels[c].bcr--;
                        }
                    }
                    else
                    {                        
                        printf("0x%04x\n", channels[c].bcr);
                        printf("[emu/IopDma]: Should start channel %d\n", c);
                        exit(1);
                    }
                }
            }
        }
    }
}