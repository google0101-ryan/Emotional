#include <emu/sif.h>
#include <app/Application.h>

void SubsystemInterface::write(uint32_t addr, uint32_t data)
{
    auto comp = (addr >> 9) & 0x1;
    uint16_t offset = (addr >> 4) & 0xF;
    auto ptr = (uint32_t*)&regs + offset;

    printf("[emu/Sif%s]: Writing 0x%08x to 0x%08x\n", comp ? "EE" : "Iop", data, addr);

    if (offset == 4)
    {
        auto& ctrl = regs.ctrl;
        if (comp == 0)
        {
            uint8_t temp = data & 0xF0;
            if (data & 0xA0)
            {
                ctrl &= ~0xF000;
                ctrl |= 0x2000;
            }

            if (ctrl & temp)
                ctrl &= ~temp;
            else
                ctrl |= temp;
        }
        else
        {
            if (!(data & 0x100))
                ctrl &= ~0x100;
            else
                ctrl |= 0x100;
        }
    }
    else
    {
        *ptr = data;
    }
}

uint32_t SubsystemInterface::read(uint32_t addr)
{
    auto comp = (addr >> 9) & 0x1;
    uint16_t offset = (addr >> 4) & 0xF;
    auto ptr = (uint32_t*)&regs + offset;

    if (!comp)
        printf("[emu/Sif%s]: Reading 0x%08x from 0x%08x\n", comp ? "EE" : "Iop", *ptr, addr);

    return *ptr;
}

uint32_t SubsystemInterface::pop_fifo()
{
    uint32_t data = fifo0.front();
    fifo0.pop();
    return data;
}

uint64_t SubsystemInterface::pop_fifo64()
{
    uint32_t data[2];
    for (int i = 0; i < 2; i++)
    {
        data[i] = pop_fifo();
    }

    return *(uint64_t*)data;
}