#include <emu/gs/gif.h>
#include "gif.h"

void GIF::process_packed(uint128_t qword)
{
	int curr_reg = tag.nreg - regs_left;
	uint64_t regs = tag.regs;
	uint32_t desc = (regs >> 4 * curr_reg) & 0xf;

	printf("[emu/GIF]: PACKED write to 0x%08x\n", desc);

	switch (desc)
	{
	case 0x00:
		gpu->prim = (qword.u64[0] & 0x7FF);
		break;
	case 0x01:
	{
		GraphicsSynthesizer::RGBAQReg target;
		target.r = qword.u128 & 0xff;
		target.g = (qword.u128 >> 32) & 0xff;
		target.b = (qword.u128 >> 64) & 0xff;
		target.a = (qword.u128 >> 96) & 0xff;
		
		gpu->write(0x1, target.value);
		break;
	}
	case 0x02:
	{
		gpu->write(0x2, qword.u64[0]);
		internal_Q = qword.u64[1];
		break;
	}
	case 0x04:
	{
		bool disable_draw = (qword.u128 >> 111) & 1;
		auto address = disable_draw ? 0xc : 0x4;

		GraphicsSynthesizer::XYZF target;
		target.x = qword.u128 & 0xffff;
		target.y = (qword.u128 >> 32) & 0xffff;
		target.z = (qword.u128 >> 68) & 0xffffff;
		target.f = (qword.u128 >> 100) & 0xff;

		gpu->write(address, target.value);
		break;
	}
	case 0x05:
	{
		bool disable_draw = (qword.u128 >> 111) & 1;
		auto address = disable_draw ? 0xd : 0x5;

		GraphicsSynthesizer::XYZ target;
		target.x = qword.u128 & 0xffff;
		target.y = (qword.u128 >> 32) & 0xffff;
		target.z = (qword.u128 >> 64) & 0xffffffff;

		gpu->write(address, target.value);
	}
	case 0x0E:
	{
		uint8_t addr = (qword.u128 >> 64) & 0xFF;
		uint64_t data = (qword.u128 & UINT64_MAX);
		gpu->write(addr, data);
		break;
	}
	case 0x0F:
		break;
	default:
		gpu->write(desc, qword.u64[0]);
		break;
	}

	regs_left--;
	if (!regs_left)
	{
		regs_left = regs_count;
		data_left--;
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
				break;
			case 1:
				process_reglist(fifo.front());
				fifo.pop();
				break;
			case 2:
				printf("Transfering quad-word to VRAM\n");
				gpu->write_hwreg(fifo.front().u64[0]);
				gpu->write_hwreg(fifo.front().u64[1]);
				fifo.pop();
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