#include "sio2.h"

#include <cassert>
#include <emu/memory/Bus.h>

uint32_t sio2_ctrl = 0;

void SIO2::WriteCtrl(uint32_t data)
{
    sio2_ctrl = data;

    if (sio2_ctrl & 1)
    {
        Bus::TriggerIOPInterrupt(17);
        sio2_ctrl &= ~1;
    }

    if (data & 0xc)
    {
        printf("[emu/SIO2]: Reset\n");
    }
}