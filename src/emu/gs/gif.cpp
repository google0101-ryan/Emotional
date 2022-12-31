#include <emu/gs/gif.h>
#include "gif.h"

void GIF::process_packed(uint128_t qword)
{
	uint64_t reg_offset = (regs_count - regs_left) << 2;
	uint8_t reg = (tag.regs >> reg_offset) & 0xf;

	switch (reg)
	{
	case 0x00:
		gpu->prim = (qword.u64[0] & 0x7FF);
		break;
	case 0x0D:
		gpu->write(0x0D, qword.u128 & 0x7FF);
		break;
	case 0x0E:
	{
		uint8_t addr = (qword.u128 >> 64) & 0xFF;
		uint64_t data = (qword.u128 & UINT64_MAX);
		gpu->write(addr, data);
		break;
	}
	default:
		gpu->write(reg, qword.u64[0]);
		break;
	}
}

void GIF::process_reglist(uint128_t qword)
{
	for (int i = 0; i < 2; i++)
	{
		uint64_t reg_offset = (regs_count - regs_left) << 2;
		uint8_t reg = (tag.regs >> reg_offset) & 0xf;

		if (reg != 0xE)
			gpu->write(reg, qword.u64[i]);
		
		regs_left--;
		if (!regs_left)
		{
			regs_left = regs_count;
			data_left--;

			if (!data_left && i == 0)
				return;
		}
	}
}

GIF::GIF(GraphicsSynthesizer *gs)
{
	gpu = gs;
}

void GIF::tick(int cycles)
{
	while (!fifo.empty() && cycles--)
	{
		if (!data_left)
		{
			auto qword = fifo.front();
			fifo.pop();
			
			tag.value = qword.u128;

			printf("[emu/GIF]: Processing GIF packet NLOOP: %d, EOP: %d, PRIM_EN: %d, PRIM: 0x%08lx, FORMAT: %d, NREGS: %d, REGS: 0x%08lx\n", tag.nloop, tag.eop, tag.pre, tag.prim, tag.flg, tag.nreg, tag.regs);

			if (tag.pre)
				gpu->prim = tag.prim;
			
			data_left = tag.nloop;
			regs_left = regs_count = tag.nreg;

			internal_Q = 1.0f;

			if (data_left != 0)
			{
				gpu->priv_regs.csr.fifo = 2;
			}
		}
		else
		{
			switch (tag.flg)
			{
			case 0:
				process_packed(fifo.front());
				fifo.pop();
				regs_left--;
				if (!regs_left)
				{
					regs_left = regs_count;
					data_left--;
				}
				break;
			case 1:
				process_reglist(fifo.front());
				fifo.pop();
				break;
			case 2:
				data_left--;
				break;
			default:
				printf("Unknown GIF tag format %d\n", tag.flg);
				exit(1);
			}
		}
		if (!data_left && tag.eop)
		{
			gpu->priv_regs.csr.fifo = 1;
		}
	}
}