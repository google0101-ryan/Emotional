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
	printf("[emu/Dmac]: Write 0x%08x to %s (0x%08x) of channel %d\n", data, REGS[reg], addr, chan);

	if (channels[chan].control.running)
	{
		printf("Started transfer on channel %d\n", chan);
	}
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

	printf("[emu/DMAC]: Writing 0x%08x to %s\n", data, GLOBALS[offset]);
}

uint32_t EmotionDma::read_dma(uint32_t addr)
{
    uint32_t offset = (addr >> 4) & 0xf;
	auto ptr = (uint32_t*)&globals + offset;
	
	printf("[emu/DMAC]: Reading %s\n", GLOBALS[offset]);

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
				if (channel.qword_count > 0)
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

							uint128_t qword = *(uint128_t*)data;
							uint64_t upper = qword.u64[1];
							uint64_t lower = qword.u64[0];
							printf("[emu/DMAC]: SIF0: 0x%x%016x\n", upper, lower);

							bus->write<__uint128_t>(channel.address, qword.u128);

							channel.qword_count--;
							channel.address += 16;
						}
						break;
					}
					default:
						printf("Unknown channel with qword > 0 %d\n", id);
						exit(1);
					}
				}
				else if (channel.end_transfer)
				{
					printf("[emu/DMAC]: End transfer on channel %d\n", id);

					channel.end_transfer = false;
					channel.control.running = 0;

					globals.d_stat.channel_irq |= (1 << id);

					if (globals.d_stat.channel_irq & globals.d_stat.channel_irq_mask)
					{
						printf("\n[emu/DMAC]: INT1\n");
						exit(1);
					}
				}
				else
				{
					fetch_tag(id);
				}
			}
		}
	}
}

void EmotionDma::fetch_tag(int id)
{
	DMATag tag;
	auto& channel = channels[id];

	switch (id)
	{
	case CHANNELS::SIF0:
	{
		auto sif = bus->GetSif();
		if (sif->fifo0.size() >= 2)
		{
			uint32_t data[2] = {};
			for (int i = 0; i < 2; i++)
			{
				data[i] = sif->fifo0.front();
				sif->fifo0.pop();
			}

			tag.value = *(uint64_t*)data;
			printf("Read SIF0 DMA tag 0x%lx\n", (uint64_t)tag.value);

			channel.qword_count = tag.qwords;
			channel.control.tag = (tag.value >> 16) & 0xffff;
			channel.address = tag.address;
			channel.tag_address.address += 16;

			if (channel.control.enable_irq_bit && tag.irq)
				channel.end_transfer = true;
		}
		break;
	}
	default:
		printf("Unknown channel tag fetch %d\n", id);
		exit(1);
	}
}

uint32_t EmotionDma::read(uint32_t addr)
{
    uint32_t id = (addr >> 8) & 0xff;
	uint32_t channel = get_channel(id);
	uint32_t offset = (addr >> 4) & 0xf;
	auto ptr = (uint32_t*)&channels[channel] + offset;

	printf("[emu/DMAC]: Reading %s from channel %d\n", REGS[offset], channel);

	return *ptr;
}