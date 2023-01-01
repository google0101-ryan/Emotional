#include <emu/cpu/ee/timers.h>
#include <emu/Bus.h>
#include "timers.h"

EmotionTimers::EmotionTimers(Bus *bus)
: bus(bus)
{
}

static const char* REGS[] =
{
	"TN_COUNT",
	"TN_MODE",
	"TN_COMP",
	"TN_HOLD"
};

static const char* CLOCK[] =
{
	"BUSCLK",
	"BUSCLK / 16",
	"BUSCLK / 256",
	"HBLANK"
};

uint32_t cur_cycle[3] = {0};

void EmotionTimers::tick(uint32_t cycles)
{
	cur_cycle[0] += cycles;
	cur_cycle[1] += cycles;
	cur_cycle[2] += cycles;
 
	for (uint32_t i = 0; i < 3; i++)
	{
		auto& timer = timers[i];

		if (!timer.mode.enable)
			return;
		
		if (!(cur_cycle[i] % timer.ratio))
		{
			uint32_t old_count = timer.counter;
			timer.counter++;

			if (timer.counter >= timer.compare && old_count < timer.compare)
			{
				if (timer.mode.cmp_intr_enable && !timer.mode.cmp_flag)
				{
					bus->trigger(9 + i);
					timer.mode.cmp_flag = 1;
				}

				if (timer.mode.clear_when_cmp)
				{
					timer.counter = 0;
				}
			}

			if (timer.counter > 0xffff)
			{
				if (timer.mode.overflow_intr_enable && ~timer.mode.overflow_flag)
				{
					bus->trigger(9 + i);
					timer.mode.overflow_flag = 1;
				}

				timer.counter -= 0xffff;
			}

			cur_cycle[i] = 0;
		}
	}
}

uint32_t EmotionTimers::read(uint32_t addr)
{
	int num = (addr & 0xff00) >> 11;
	uint32_t offset = (addr & 0xf0) >> 4;
	auto ptr = (uint32_t*)&timers[num] + offset;

	//printf("[emu/Timers]: Reading from %s of timer %d\n", REGS[offset], num);

	return *ptr;
}

void EmotionTimers::write(uint32_t addr, uint32_t value)
{
	int num = (addr & 0xff00) >> 11;
	uint32_t offset = (addr & 0xf0) >> 4;
	auto ptr = (uint32_t*)&timers[num] + offset;

	// printf("[emu/Timers]: Writing to %s of timer %d\n", REGS[offset], num);

	if (offset == 1)
	{
		auto& timer = timers[num];

		switch (value & 3)
		{
		case 0:
			timer.ratio = 1; break;
		case 1:
			timer.ratio = 16; break;
		case 2:
			timer.ratio = 256; break;
		case 3:
			timer.ratio = 9371; break;
		}

		value &= 0x3ff;

		// printf("[emu/Timers]: Setting timer %d clock to %s\n", num, CLOCK[timer.mode.clock]);
	}

	*ptr = value;
}