#include <emu/cpu/ee/dmac.h>
#include <app/Application.h>
#include <emu/Bus.h>

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

EmotionDma::EmotionDma(Bus* bus)
: bus(bus)
{}

void EmotionDma::write(uint32_t addr, uint32_t data)
{
    int chan = ((addr & 0xFF00) >> 8);

    chan = get_channel(chan);
    int reg = (addr >> 4) & 0xf;

    switch (reg)
    {
    case 0x0:
        channels[chan].chcr.value = data;
        printf("Setting channel %d chcr to 0x%x (%d)\n", chan, data, channels[chan].chcr.running);
        break;
    case 0x01:
        printf("Setting channel %d maddr -> 0x%08x\n", chan, data);
        channels[chan].addr = data;
        break;
    case 0x02:
        printf("Setting channel %d qwc -> 0x%08x\n", chan, data);
        channels[chan].qwords = data;
        break;
    case 0x03:
        printf("Setting channel %d tadr -> 0x%08x\n", chan, data);
        channels[chan].addr = data;
        break;
    case 0x04:
        printf("Setting channel %d asr0 -> 0x%08x\n", chan, data);
        channels[chan].asr0 = data;
        break;
    case 0x05:
        printf("Setting channel %d asr1 -> 0x%08x\n", chan, data);
        channels[chan].asr1 = data;
        break;
    case 0x8:
        printf("Setting channel %d spr -> 0x%08x\n", chan, data);
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

            if (chan->chcr.running)
            {
                if (chan->qwords > 0)
                {
                    printf("Need to start channel %d (from 0x%08x)\n", i, chan->addr);
                    Application::Exit(1);
                }
                else if (chan->should_finish)
                {
                    chan->chcr.running = false;
                    chan->should_finish = false;
                    printf("ERROR: Need to raise an IRQ here\n");
                }
                else
                {
                    switch (i)
                    {
                    case CHANNELS::SIF0:
                    {
                        if (bus->GetSif()->fifo0.size() >= 2)
                        {
                            DMATag tag;
                            tag.value = bus->GetSif()->pop_fifo64();

                            printf("Grabbed tag for channel %d -> 0x%08lx\n", i, tag.value);

                            chan->addr = tag.addr;
                            chan->qwords = tag.qword_count;
                            chan->chcr.tag = (tag.value >> 16) & 0xffff;
                            chan->save_tag_addr += 16;

                            if (chan->chcr.enable_irq_bit && tag.irq)
                                chan->should_finish = true;
                        }
                    }
                    break;
                    default:
                        printf("[emu/Dmac]: %s: Failed to get tag for channel %d\n", __FUNCTION__, i);
                        Application::Exit(1);
                    }
                }
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
        return channels[chan].addr;
    case 0x02:
        return channels[chan].qwords;
    case 0x03:
        return channels[chan].addr;
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