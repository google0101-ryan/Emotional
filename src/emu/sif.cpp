#include <emu/sif.h>
#include <app/Application.h>

void SubsystemInterface::write_ee(uint32_t addr, uint32_t data)
{
    switch (addr)
    {
    case 0x1000F200:
        mscom = data;
        break;
    case 0x1000F220:
        msflg = data;
        if (msflg == smflg)
            smflg = 0; // Writing the value of smflg to msflg clears smflg
        break;
    case 0x1000F240:
        sif_ctrl = data;
        break;
    case 0x1000F260:
        sif_bd6 = data;
        break;
    default:
        printf("[emu/Sif]: %s: Write to unknown addr 0x%08x\n", __FUNCTION__, addr);
        Application::Exit(1);
    }
}

uint32_t SubsystemInterface::read_ee(uint32_t addr)
{
    switch (addr)
    {
    case 0x1000F200:
        return mscom;
    case 0x1000F210:
        return smcom;
    case 0x1000F220:
        return msflg;
    case 0x1000F230:
        return 0x10000;
    default:
        printf("[emu/Sif]: %s: Read from unknown addr 0x%08x\n", __FUNCTION__, addr);
        Application::Exit(1);
    }
}