#include <emu/cpu/ee/dmac.h>
#include <app/Application.h>
#include <emu/Bus.h>
#include "dmac.h"

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

		if (globals.d_stat.channel_irq & globals.d_stat.channel_irq_mask)
			bus->TriggerDMAInterrupt();
		else
			bus->ClearDMAInterrupt();
	}
	else
		*ptr = data;

	printf("[emu/DMAC]: Writing 0x%08x to %s\n", data, GLOBALS[offset]);
}

uint32_t EmotionDma::read_dma(uint32_t addr)
{
    uint32_t offset = (addr >> 4) & 0xf;
	auto ptr = (uint32_t*)&globals + offset;
	
	//printf("[emu/DMAC]: Reading %s (0x%08x)\n", GLOBALS[offset], *ptr);

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
					case CHANNELS::VIF1:
					{
						auto vif = bus->GetVIF(1);
						__uint128_t qword = *(__uint128_t*)&bus->grab_ee_ram()[channel.address];
						if (vif->write_fifo(qword))
						{
							channel.address += 16;
							channel.qword_count--;

							if (!channel.qword_count)
								channel.end_transfer = true;
						}
						break;
					}
					case CHANNELS::GIF:
					{
						auto gif = bus->GetGIF();
						uint128_t qword;
						qword.u128 = *(__uint128_t*)&bus->grab_ee_ram()[channel.address];

						printf("[emu/DMAC]: Transferring GIF qword 0x%lx%016lx\n", qword.u64[1], qword.u64[0]);
						
						gif->write32(0x10006000, qword);
						channel.qword_count--;
						channel.address += 16;
						if (!channel.qword_count)
							channel.end_transfer = true;
						break;
					}
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
							printf("[emu/DMAC]: SIF0: 0x%lx%016lx\n", upper, lower);

							bus->write<__uint128_t>(channel.address, qword.u128);

							channel.qword_count--;
							channel.address += 16;
						}
						break;
					}
					case CHANNELS::SIF1:
					{
						auto sif = bus->GetSif();

						__uint128_t qword = *(__uint128_t*)&bus->grab_ee_ram()[channel.address];
						uint32_t* data = (uint32_t*)&qword;
						for (int i = 0; i < 4; i++)
						{
							sif->fifo1.push(data[i]);
						}

						uint64_t upper = qword >> 64, lower = qword;
						printf("[DMAC][SIF1] Transfering to SIF1 FIFO 0x%lx%016lx\n", upper, lower);

						channel.qword_count--;
						channel.address += 16;

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
						printf("Triggering end-of-channel interrupt\n");
						bus->TriggerDMAInterrupt();
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

uint32_t EmotionDma::read_enabler()
{
	return globals.d_enable;
}

void EmotionDma::write_enabler(uint32_t data)
{
	globals.d_enable = data;
}

enum DMASourceID : uint32_t
{
	REFE,
	CNT,
	NEXT,
	REF,
	REFS,
	CALL,
	RET,
	END
};

void EmotionDma::fetch_tag(int id)
{
	DMATag tag;
	auto& channel = channels[id];

	switch (id)
	{
	case CHANNELS::GIF:
	{
		auto gif = bus->GetGIF();
		auto address = channel.tag_address.address;

		tag.value = *(__uint128_t*)&bus->grab_ee_ram()[address];

		printf("[emu/DMAC]: New GIF transfer: %d qwords\n", tag.qwords);

		channel.qword_count = tag.qwords;
		channel.control.tag = (tag.value >> 16) & 0xffff;

		uint16_t tag_id = tag.id;
		switch (tag_id)
		{
		case DMASourceID::CNT:
			channel.address = channel.tag_address.address + 16;
			channel.tag_address.value = channel.address + channel.qword_count * 16;
			break;
		default:
			printf("Unknown DMA tag id %d\n", tag_id);
			exit(1);
		}

		if (channel.control.enable_irq_bit && tag.irq)
			channel.end_transfer = true;

		break;
	}
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
	case CHANNELS::SIF1:
	{
		auto address = channel.tag_address.address;

		tag.value = *(__uint128_t*)&bus->grab_ee_ram()[address];
		printf("[emu/DMAC]: Read SIF1 DMA tag 0x%08lx\n", (uint64_t)tag.value);

		channel.qword_count = tag.qwords;
		channel.control.tag = (tag.value >> 16) & 0xffff;

		uint16_t tag_id = tag.id;
		switch (tag_id)
		{
		case DMASourceID::REFE:
			channel.address = tag.address;
			channel.tag_address.value += 16;
			channel.end_transfer = true;
			break;
		case DMASourceID::CNT:
			channel.address = channel.tag_address.address + 16;
			channel.tag_address.value = channel.address + channel.qword_count * 16;
			break;
		case DMASourceID::NEXT:
			channel.address = channel.tag_address.address + 16;
			channel.tag_address.value = tag.address;
			break;
		case DMASourceID::REF:
			channel.address = tag.address;
			channel.tag_address.value += 16;
			break;
		case DMASourceID::END:
			channel.address = channel.tag_address.address + 16;
			channel.end_transfer = true;
			break;
		default:
			printf("\n[DMAC] Unrecognized SIF1 DMAtag id %d\n", tag_id);
			exit(1);
		}

		if (channel.control.enable_irq_bit && tag.irq)
			channel.end_transfer = true;

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

	//printf("[emu/DMAC]: Reading %s from channel %d\n", REGS[offset], channel);

	return *ptr;
}