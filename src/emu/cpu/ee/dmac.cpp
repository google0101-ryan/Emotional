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

constexpr const char* REGS[] =
{
	"Dn_CHCR",
	"Dn_MADR",
	"Dn_QWC",
	"Dn_TADR",
	"Dn_ASR0",
	"Dn_ASR1",
	"", "",
	"Dn_SADR"
};

constexpr const char* GLOBALS[] =
{
	"D_CTRL",
	"D_STAT",
	"D_PCR",
	"D_SQWC",
	"D_RBSR",
	"D_RBOR",
	"D_STADT",
};

void EmotionDma::write(uint32_t addr, uint32_t data)
{
    int id = (addr >> 8) & 0xff;
	uint32_t chan = get_channel(id);
    int reg = (addr >> 4) & 0xf;

    auto ptr = (uint32_t*)&channels[chan] + reg;

	if (reg == 1)
		data &= 0x01fffff0;
	
	*ptr = data;

	if (channels[chan].control.running)
	{
		printf("Started transfer on channel %d\n", chan);
	}

	
	printf("[emu/Dmac]: Write 0x%08x to %s (0x%08x) of channel %d\n", data, REGS[reg], addr, chan);
}

void EmotionDma::write_dma(uint32_t addr, uint32_t data)
{
	uint32_t offset = (addr >> 4) & 0xf;
	auto ptr = (uint32_t*)&globals + offset;

	if (offset == 1)
	{
		globals.d_stat.clear &= ~(data & 0xffff);
		globals.d_stat.reverse ^= (data >> 16);
	}
	else
		*ptr = data;
}

uint32_t EmotionDma::read_dma(uint32_t addr)
{
    uint32_t offset = (addr >> 4) & 0xf;
	auto ptr = (uint32_t*)&globals + offset;
	
	return *ptr;
}

void EmotionDma::tick(int cycles)
{
	if (globals.d_enable & 0x10000)
		return;
	
	for (int cycle = cycles; cycle > 0; cycle--)
	{
		for (uint32_t id = 0; id < 10; id++)
		{
			auto& channel = channels[id];
			if (channel.control.running)
			{
				switch (id)
				{
				case CHANNELS::SIF0:
				{
					auto sif = bus->GetSif();
					if (sif->fifo0.size() >= 4)
					{
						uint32_t data[4];
						for (int i = 0; i < 4; i++)
						{
							data[i] = sif->fifo0.front();
							sif->fifo0.pop();
						}

						__uint128_t qword = *(__uint128_t*)data;
						uint64_t upper = qword >> 64, lower = qword;
						printf("[emu/IoDma]: Receiving packet from SIF0 0x%lx%016lx\n", upper, lower);

						bus->write<__uint128_t>(channel.address, qword);

						channel.qword_count--;
						channel.address += 16;
					}
					break;
				}
				default:
					printf("Unknown channel %d (0x%04x)\n", id, channel.qword_count);
					exit(1);
				}
			}
		}
	}
}

uint32_t EmotionDma::read(uint32_t addr)
{
    uint32_t id = (addr >> 8) & 0xff;
	uint32_t channel = get_channel(id);
	uint32_t offset = (addr >> 4) & 0xf;
	auto ptr = (uint32_t*)&channels[channel] + offset;

	return *ptr;
}