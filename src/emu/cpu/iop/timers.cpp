#include <emu/cpu/iop/timers.h>
#include <emu/iopbus.h>
#include <cstdio>
#include <cstdlib>

void Timer::tick(uint32_t cycles)
{
    auto& timer = timers[5];

    uint32_t old_count = timer.counter;
    timer.counter += cycles;

    if (timer.counter >= timer.target && old_count < timer.target)
    {
        timer.mode.compare_intr_raised = true;
        if (timer.mode.compare_intr && timer.mode.intr_enabled)
        {
            bus->TriggerInterrupt(16);
        }
        if (timer.mode.reset_on_intr)
        {
            timer.counter = 0;
        }
    }
    if (timer.counter > 0xFFFFFFFF)
    {
        timer.mode.overflow_intr_raised = true;
        if (timer.mode.overflow_intr && timer.mode.intr_enabled)
        {
            bus->TriggerInterrupt(16);
        }
        timer.counter -= 0xFFFFFFFF;
    }
}

uint32_t Timer::read(uint32_t addr)
{
    bool group = addr & 0x400;
    uint32_t timer = ((addr & 0x30) >> 4) + 3 * group;
    uint32_t offset = (addr & 0xf) >> 2;

    auto ptr = (uint64_t*)&timers[timer] + offset;
    auto value = *ptr;
    if (offset == 1)
    {
        TimerMode& mode = timers[timer].mode;
        mode.compare_intr_raised = false;
        mode.overflow_intr_raised = false;
    }
    return value;
}

void Timer::write(uint32_t addr, uint32_t data)
{
    bool group = addr & 0x400;
	uint32_t timer = ((addr & 0x30) >> 4) + 3 * group;
	uint32_t offset = (addr & 0xf) >> 2;
	auto ptr = (uint64_t*)&timers[timer] + offset;
	*ptr = data;

    switch (offset)
    {
    case 1:
    {
        TimerMode& mode = timers[timer].mode;
        mode.intr_enabled = true;
        timers[timer].counter = 0;
        break;
    }
    case 2:
    {
        TimerMode& mode = timers[timer].mode;
        mode.intr_enabled = (mode.levl ? mode.intr_enabled : 1);
    }
    }
}