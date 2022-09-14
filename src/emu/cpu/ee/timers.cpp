#include <emu/cpu/ee/timers.h>

void EmotionTimers::write(uint32_t addr, uint32_t value)
{
    int num = (addr & 0xff00) >> 11;
    addr = addr & ~(0xff00);

    switch (addr)
    {
    case 0x10000000:
        timers[num].count = value;
        break;
    case 0x10000010:
        timers[num].mode = value;
        break;
    case 0x10000020:
        timers[num].comp = value;
        break;
    case 0x10000030:
        timers[num].hold = value;
        break;
    default:
        printf("[emu/Timer]: %s: Write to unknown addr 0x%08x\n", __FUNCTION__, addr);
        exit(1);
    }
}