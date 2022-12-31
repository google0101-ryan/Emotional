#include <emu/cpu/iop/sio2.h>
#include <cstdio>
#include <cstdlib>
#include <emu/iopbus.h>
#include "sio2.h"
SIO2::SIO2(IopBus *iop)
	: bus(iop)
{
}

void SIO2::process_command(uint8_t cmd)
{
	if (!command.size)
	{
		auto params = send3[command.index];
		if (!params)
		{
			printf("Empty SEND3 parameter!\n");
			exit(1);
		}
		command.size = (params >> 8) & 0x1ff;
		command.index++;

		cur_dev = (SIO2Peripheral)cmd;
		just_started = true;
	}

	switch (cur_dev)
	{
	case SIO2Peripheral::MemCard:
		recv1 = 0x1D100;
		fifo.push(0x00);
		return;
	default:
		printf("[emu/IOP]: Sending unknown command 0x%02x\n", (uint8_t)cur_dev);
		exit(1);
	}
}

uint32_t SIO2::read(uint32_t addr)
{
	switch (addr)
	{
	case 0x1f808264:
	{
		uint8_t val = fifo.front();
		fifo.pop();
		return val;
	}
	case 0x1f808268:
		return sio2_ctrl.data;
	case 0x1f80826C:
		return recv1;
	case 0x1f808270:
		return 0xF;
	case 0x1f808274:
		return 0xF;
	case 0x1f808280:
		return 0;
	default:
		printf("[emu/SIO2]: Read from unknown addr 0x%08x\n", addr);
		exit(1);
	}
}

void SIO2::write(uint32_t addr, uint32_t data)
{
	switch (addr)
	{
	case 0x1f808260:
		process_command(data);
		break;
	case 0x1f808268:
		sio2_ctrl.data = data & 1;

		if (data & 1)
		{
			bus->TriggerInterrupt(17);
			sio2_ctrl.data &= ~1;
		}
		break;
	case 0x1f808280:
		break;
	default:
		printf("[emu/SIO2]: Write to unknown addr 0x%08x\n", addr);
		exit(1);
	}
}

void SIO2::set_send1(int index, uint32_t value)
{
	send1[index] = value;
}

void SIO2::set_send2(int index, uint32_t value)
{
	send2[index] = value;
}

void SIO2::set_send3(int index, uint32_t value)
{
	send3[index] = value;
}
