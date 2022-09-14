#include <emu/cpu/ee/dmac.h>

inline uint32_t get_channel(uint32_t value)
{
	switch (value)
	{
	case 0x80: return 0;
	case 0x90: return 1;
	case 0xa0: return 2;
	case 0xb0: return 3;
	case 0xb4: return 4;
	case 0xc0: return 5;
	case 0xc4: return 6;
	case 0xc8: return 7;
	case 0xd0: return 8;
	case 0xd4: return 9;
    default:
        return 0xff;
	}
}

void EmotionDma::write(uint32_t addr, uint32_t data)
{
    int chan = ((addr & 0xFF00) >> 8);

    chan = get_channel(chan);
    int reg = (addr >> 4) & 0xf;

    switch (reg)
    {
    case 0x0:
        channels[chan].chcr.value = data;
        break;
    case 0x01:
        channels[chan].madr = data;
        break;
    case 0x02:
        channels[chan].qwords = data;
    case 0x03:
        channels[chan].tadr = data;
        break;
    case 0x04:
        channels[chan].asr0 = data;
        break;
    case 0x05:
        channels[chan].asr1 = data;
        break;
    case 0x8:
        channels[chan].scratchpad_addr = data;
        break;
    default:
        printf("[emu/DMAC]: %s: Failed to write to address 0x%08x\n", __FUNCTION__, addr);
        exit(1);
    }
}

void EmotionDma::write_dma(uint32_t addr, uint32_t data)
{
    switch (addr)
    {
    case 0x1000E000:
        ctrl.value = data;
        break;
    case 0x1000E010:
        stat.value = data;
        break;
    case 0x1000E020:
        prio = data;
        break;
    case 0x1000E030: // Ignore sqwc, because we don't do interleaving
        break;
    case 0x1000E040:
        mfifo_size = data;
        break;
    case 0x1000E050:
        mfifo_addr = data;
        break;
    default:
        printf("[emu/DMAC]: %s: Failed to write to address 0x%08x\n", __FUNCTION__, addr);
        exit(1);
    }
}

uint32_t EmotionDma::read_dma(uint32_t addr)
{
    switch (addr)
    {
    case 0x1000E000:
        return ctrl.value;
    case 0x1000E010:
        return stat.value;
    case 0x1000E020:
        return prio;
    default:
        printf("[emu/DMAC]: %s: Failed to read from address 0x%08x\n", __FUNCTION__, addr);
        exit(1);
    }
}

void EmotionDma::tick(int cycles)
{
    while (cycles--)
    {
        if (!ctrl.dma_enable)
            return;
        Channel* chan;
        for (int i = 0; i < 10; i++)
        {
            chan = &channels[i];

            if (chan->chcr.start)
            {
                printf("Need to start channel %d\n", i);
                exit(1);
            }
        }
    }
}

uint32_t EmotionDma::read(uint32_t addr)
{
    int chan = ((addr & 0xFF00) >> 8);

    chan = get_channel(chan);
    int reg = (addr >> 4) & 0xf;

    switch (reg)
    {
    case 0x0:
        return channels[chan].chcr.value;
    case 0x01:
        return channels[chan].madr;
    case 0x02:
        return channels[chan].qwords;
    case 0x03:
        return channels[chan].tadr;
        break;
    case 0x04:
        return channels[chan].asr0;
    case 0x05:
        return channels[chan].asr1;
    case 0x8:
        return channels[chan].scratchpad_addr;
    default:
        printf("[emu/DMAC]: %s: Failed to read from address 0x%08x\n", __FUNCTION__, addr);
        exit(1);
    }
}