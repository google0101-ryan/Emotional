#include <emu/sif.h>
#include <app/Application.h>

constexpr const char* REGS[] =
{
	"SIF_MSCOM", "SIF_SMCOM", "SIF_MSFLG",
	"SIF_SMFLG", "SIF_CTRL", "", "SIF_BD6"
};

void SubsystemInterface::write(uint32_t addr, uint32_t data)
{
    auto comp = (addr >> 9) & 0x1;
    uint16_t offset = (addr >> 4) & 0xF;
    auto ptr = (uint32_t*)&regs + offset;

    printf("[emu/Sif%s]: Writing 0x%08x to %s\n", comp ? "EE" : "Iop", data, REGS[offset]);

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

    if (!comp && addr != 0x1D000020)
		printf("[emu/Sif%s]: Reading 0x%08x from %s\n", comp ? "EE" : "Iop", *ptr, REGS[offset]);

    return *ptr;
}