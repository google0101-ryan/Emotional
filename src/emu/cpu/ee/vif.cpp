#include <emu/cpu/ee/vif.h>
#include "vif.h"

void VectorInterface::write(uint32_t addr, uint32_t data)
{
    int off = (addr >> 4) & 0xf;

    switch (off)
    {
    case 0:
        stat.fifo_detection = data & 0x800000;
        break;
    case 1:
        fbrst = data;
        break;
    case 2:
        err = data;
        break;
    case 3:
        mark = data;
		stat.mark = 0;
        break;
    default:
        printf("[emu/VIF%d]: %s: Write to unknown addr 0x%08x\n", id, __FUNCTION__, addr);
        exit(1);
    }
}

uint32_t VectorInterface::read(uint32_t addr)
{
	int off = (addr >> 4) & 0xf;

    switch (off)
    {
	case 0:
		return stat.value;
	default:
		printf("[emu/VIF%d]: Read from unknown address 0x%08x\n", id, addr);
		exit(1);
	}
}

void VectorInterface::tick(int cycles)
{
	word_cycles = cycles * 4;
	while (!fifo.empty() && word_cycles--)
	{
		if (!subpacket_count)
			process_command();
		else
			execute_command();
	}
}

void VectorInterface::process_command()
{
	command.value = fifo.front();
	fifo.pop();

	switch (command.command)
	{
	case VIFCommands::NOP:
		printf("[emu/VIF%d]: nop\n", id);
		break;
	case VIFCommands::STCYCL:
		cycle.value = command.immediate;
		printf("[emu/VIF%d]: stcycl 0x%04x\n", id, command.immediate);
		break;
	case VIFCommands::BASE:
		base = command.immediate & 0x3ff;
		printf("[emu/VIF%d]: base 0x%08x\n", id, base);
		break;
	case VIFCommands::ITOP:
		itop = command.immediate & 0x3ff;
		printf("[emu/VIF%d]: itop 0x%04x\n", id, itop);
		break;
	case VIFCommands::STMASK:
		subpacket_count = 1;
		break;
	case VIFCommands::MSKPATH3:
		break;
	default:
		printf("Unknown VIF%d command 0x%02x\n", id, command.command);
		exit(1);
	}
}

void VectorInterface::execute_command() 
{
	uint32_t data = fifo.front(); 
	fifo.pop();
	switch (command.command)
	{
	case VIFCommands::STMASK:
		mask = data;
		printf("[emu/VIF%d]: stmask 0x%08x\n", id, data);
		break;
	default:
		printf("Unknown execute VIF%d command 0x%02x\n", id, command.command);
		exit(1);
	}

	subpacket_count--;
}

bool VectorInterface::write_fifo(uint32_t data)
{
    if (fifo.size() > (fifo_size - 1))
		return false;
    
    printf("Adding 0x%08x to VIF%d fifo\n", data, id);
    fifo.push(data);
    return true;
}

bool VectorInterface::write_fifo(__uint128_t data)
{
	uint32_t* buf = (uint32_t*)&data;
	for (int i = 0; i < 4; i++)
	{
		if (!write_fifo(buf[i]))
			return false;
	}
	return true;
}