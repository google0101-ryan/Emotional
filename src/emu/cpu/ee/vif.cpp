#include <emu/cpu/ee/vif.h>

void VectorInterface::write(uint32_t addr, uint32_t data)
{
    int off = (addr >> 4) & 0xf;

    switch (off)
    {
    case 0:
        stat = data;
        break;
    case 1:
        fbrst = data;
        break;
    case 2:
        err = data;
        break;
    case 3:
        stat &= ~(1 << 6);
        break;
    default:
        printf("[emu/VIF%d]: %s: Write to unknown addr 0x%08x\n", id, __FUNCTION__, addr);
        exit(1);
    }
}

void VectorInterface::write_fifo(uint32_t data)
{
    if (fifo.size() > (fifo_size - 1))
        return;
    
    printf("Adding 0x%08x to VIF%d fifo\n", data, id);
    fifo.push(data);
    return;
}