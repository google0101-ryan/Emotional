#include "vu.h"
#include <fstream>
#include <emu/memory/Bus.h>
#include <emu/cpu/ee/EmotionEngine.h>
#include <cassert>

extern float convert(uint32_t value);

namespace VectorUnit
{

uint32_t data_address_mask[2] = {0xfff, 0x3fff};
uint32_t code_address_mask[2] = {0xfff, 0x3fff};

uint8_t data[2][0x4000];
uint8_t code[2][0x4000];

void WriteDataMem128(int vector, uint32_t addr, uint128_t _data)
{
    if (vector == 1)
        addr = (addr - 0x1100C000);
    else
        addr = (addr - 0x11004000);
    
    addr &= data_address_mask[vector];

	if (vector == 0 && (addr >= 0x4000 && addr <= 0x43ff))
	{
		printf("Unhandled write to VU1 register 0x%04x\n", addr);
		exit(1);
	}

    *(uint128_t*)&data[vector][addr] = _data;
}

void WriteDataMem32(int vector, uint32_t addr, uint32_t _data)
{
	if (vector == 1)
        addr = (addr - 0x1100C000);
    else
        addr = (addr - 0x11004000);
    
    addr &= data_address_mask[vector];

	if (vector == 0 && (addr >= 0x4000 && addr <= 0x43ff))
	{
		printf("Unhandled write to VU1 register 0x%04x\n", addr);
		exit(1);
	}

    *(uint32_t*)&data[vector][addr] = _data;
}

void WriteCodeMem128(int vector, uint32_t addr, uint128_t data)
{
	if (vector == 1)
        addr = (addr - 0x11008000);
    else
        addr = (addr - 0x11000000);
	
	addr &= code_address_mask[vector];

    *(uint128_t*)&code[vector][addr] = data;
}

void WriteCodeMem64(int vector, uint32_t addr, uint64_t data)
{
	if (vector == 1)
        addr = (addr - 0x11008000);
    else
        addr = (addr - 0x11000000);
	
	addr &= code_address_mask[vector];

    *(uint64_t*)&code[vector][addr] = data;
}

void Dump()
{
	std::ofstream out_file("vu0_code.bin");

	for (int i = 0; i < 0x4000; i++)
	{
		out_file << code[0][i];
	}

	out_file.close();
	out_file.open("vu0_data.bin");

	for (int i = 0; i < 0x4000; i++)
	{
		out_file << data[0][i];
	}

	out_file.close();
	out_file.open("vu1_code.bin");
	for (int i = 0; i < 0x4000; i++)
	{
		out_file << code[1][i];
	}

	for (int i = 0; i < 16; i++)
		printf("[emu/VU0]: VI%02d:\t->\t0x%04x\n", i, VU0::vu0_state.vi[i]);
	for (int i = 0; i < 32; i++)
		printf("[emu/VU0]: VF%02d:\t->\t{x = %f, y = %f, z = %f, w = %f}\n", i, convert(VU0::vu0_state.vf[i].xi), convert(VU0::vu0_state.vf[i].yi), convert(VU0::vu0_state.vf[i].zi), convert(VU0::vu0_state.vf[i].wi));
	printf("[emu/VU0]: I:\t->\t%f\n", VU0::vu0_state.i);
}

namespace VU0
{

VectorState vu0_state;

uint32_t ReadControl(int index)
{
	switch (index)
	{
	case 1 ... 15:
		return vu0_state.vi[index];
	case 28:
		return vu0_state.fbrst;
	default:
		printf("[emu/VU0]: Read from unknown control register %d\n", index);
		exit(1);
	}
}

void WriteControl(int index, uint32_t data)
{
	switch (index)
	{
	case 1 ... 15:
		printf("Writing 0x%08x to vi%02d\n", data, index);
		vu0_state.vi[index] = data;
		break;
	case 16:
		vu0_state.status = data & 0xFFF;
		break;
	case 18:
		vu0_state.clipping.val = data & 0xFFFFFF;
		break;
	case 20:
		vu0_state.r = data & 0x7FFFFF;
		break;
	case 21:
		vu0_state.i = data;
		break;
	case 22:
		vu0_state.q = data;
		break;
	case 27:
		vu0_state.cmsar0 = data;
		break;
	case 28:
		vu0_state.fbrst = data & 0xC0C;
		break;
	default:
		printf("[emu/VU0]: Write to unknown control register %d\n", index);
		exit(1);
	}
}

__uint128_t ReadReg(int index)
{
    return vu0_state.vf[index].u128.u128;
}

void WriteReg(int index, __uint128_t val)
{
	printf("VU0: Writing %s to vf%02d\n", print_128({val}), index);
	vu0_state.vf[index].u128.u128 = val;
}

void LQC2(uint32_t instr)
{
	int32_t offs = (int32_t)(int16_t)(instr & 0xffff);

	uint8_t base = (instr >> 21) & 0x1F;
	uint8_t ft = (instr >> 16) & 0x1F;

	vu0_state.vf[ft].u128 = Bus::Read128(EmotionEngine::GetState()->regs[base].u32[0]+offs);
}

void SQC2(uint32_t instr)
{
	int32_t offs = (int32_t)(int16_t)(instr & 0xffff);

	uint8_t base = (instr >> 21) & 0x1F;
	uint8_t ft = (instr >> 16) & 0x1F;

	Bus::Write128(EmotionEngine::GetState()->regs[base].u32[0]+offs, vu0_state.vf[ft].u128);
}

void Vmaddz(uint32_t instr)
{
	uint8_t dest = (instr >> 21) & 0xf;
	bool x = (dest >> 3) & 1;
	bool y = (dest >> 2) & 1;
	bool z = (dest >> 1) & 1;
	bool w = (dest >> 0) & 1;

	std::string dest_;
	if (x)
		dest_ += "x";
	if (y)
		dest_ += "y";
	if (z)
		dest_ += "z";
	if (w)
		dest_ += "w";
	
	int fd = (instr >> 6) & 0x1F;
	int ft = (instr >> 16) & 0x1F;
	int fs = (instr >> 11) & 0x1F;

	float op = convert(vu0_state.vf[ft].zi);

	if (x)
	{
		vu0_state.vf[fd].x = convert(vu0_state.acc.xi) + (convert(vu0_state.vf[fs].xi) * op);
	}
	if (y)
	{
		vu0_state.vf[fd].y = convert(vu0_state.acc.yi) + (convert(vu0_state.vf[fs].yi) * op);
	}
	if (z)
	{
		vu0_state.vf[fd].z = convert(vu0_state.acc.zi) + (convert(vu0_state.vf[fs].zi) * op);
	}
	if (w)
	{
		vu0_state.vf[fd].w = convert(vu0_state.acc.wi) + (convert(vu0_state.vf[fs].wi) * op);
	}

	printf("vmaddz.%s vf%02d.%s, vf%02d.%s, vf%02dz\n", dest_.c_str(), fd, dest_.c_str(), fs, dest_.c_str(), ft);
}

void Vmaddw(uint32_t instr)
{
	uint8_t dest = (instr >> 21) & 0xf;
	bool x = (dest >> 3) & 1;
	bool y = (dest >> 2) & 1;
	bool z = (dest >> 1) & 1;
	bool w = (dest >> 0) & 1;

	std::string dest_;
	if (x)
		dest_ += "x";
	if (y)
		dest_ += "y";
	if (z)
		dest_ += "z";
	if (w)
		dest_ += "w";
	
	int fd = (instr >> 6) & 0x1F;
	int ft = (instr >> 16) & 0x1F;
	int fs = (instr >> 11) & 0x1F;

	float op = convert(vu0_state.vf[ft].wi);

	if (x)
	{
		vu0_state.vf[fd].x = convert(vu0_state.acc.xi) + (convert(vu0_state.vf[fs].xi) * op);
	}
	if (y)
	{
		vu0_state.vf[fd].y = convert(vu0_state.acc.yi) + (convert(vu0_state.vf[fs].yi) * op);
	}
	if (z)
	{
		vu0_state.vf[fd].z = convert(vu0_state.acc.zi) + (convert(vu0_state.vf[fs].zi) * op);
	}
	if (w)
	{
		vu0_state.vf[fd].w = convert(vu0_state.acc.wi) + (convert(vu0_state.vf[fs].wi) * op);
	}

	printf("vmaddw.%s vf%02d.%s, vf%02d.%s, vf%02dw\n", dest_.c_str(), fd, dest_.c_str(), fs, dest_.c_str(), ft);
}

void Vsub(uint32_t instr)
{
	uint8_t dest = (instr >> 21) & 0xf;
	bool x = (dest >> 3) & 1;
	bool y = (dest >> 2) & 1;
	bool z = (dest >> 1) & 1;
	bool w = (dest >> 0) & 1;

	std::string dest_;
	if (x)
		dest_ += "x";
	if (y)
		dest_ += "y";
	if (z)
		dest_ += "z";
	if (w)
		dest_ += "w";

	int ft = (instr >> 16) & 0x1F;
	int fs = (instr >> 11) & 0x1F;
	int fd = (instr >> 6) & 0x1F;

	if (x)
		vu0_state.vf[fd].x = convert(vu0_state.vf[fs].xi) - convert(vu0_state.vf[ft].xi);
	if (y)
		vu0_state.vf[fd].y = convert(vu0_state.vf[fs].yi) - convert(vu0_state.vf[ft].yi);
	if (z)
		vu0_state.vf[fd].z = convert(vu0_state.vf[fs].zi) - convert(vu0_state.vf[ft].zi);
	if (w)
		vu0_state.vf[fd].w = convert(vu0_state.vf[fs].wi) - convert(vu0_state.vf[ft].wi);
	
	printf("vsub.%s vf%02d.%s, vf%02d.%s, vf%02d.%s\n", dest_.c_str(), fd, dest_.c_str(), fs, dest_.c_str(), ft, dest_.c_str());

	// TODO: Handle VU0 flags
}

void Viadd(uint32_t instr)
{
	int it = (instr >> 16) & 0x1F;
	int is = (instr >> 11) & 0x1F;
	int id = (instr >> 6) & 0x1F;

	vu0_state.vi[id] = vu0_state.vi[is] + vu0_state.vi[it];

	printf("viadd vi%02d, vi%02d, vi%02d\n", id, is, it);
}

void Vmaddax(uint32_t instr)
{
	uint8_t dest = (instr >> 21) & 0xf;
	bool x = (dest >> 3) & 1;
	bool y = (dest >> 2) & 1;
	bool z = (dest >> 1) & 1;
	bool w = (dest >> 0) & 1;

	std::string dest_;
	if (x)
		dest_ += "x";
	if (y)
		dest_ += "y";
	if (z)
		dest_ += "z";
	if (w)
		dest_ += "w";
	
	int ft = (instr >> 16) & 0x1F;
	int fs = (instr >> 11) & 0x1F;

	float op = convert(vu0_state.vf[ft].xi);

	if (x)
	{
		vu0_state.acc.x = convert(vu0_state.acc.xi) + (convert(vu0_state.vf[fs].xi) * op);
	}
	if (y)
	{
		vu0_state.acc.y = convert(vu0_state.acc.yi) + (convert(vu0_state.vf[fs].yi) * op);
	}
	if (z)
	{
		vu0_state.acc.z = convert(vu0_state.acc.zi) + (convert(vu0_state.vf[fs].zi) * op);
	}
	if (w)
	{
		vu0_state.acc.w = convert(vu0_state.acc.wi) + (convert(vu0_state.vf[fs].wi) * op);
	}

	printf("vmaddax.%s acc, vf%02d.%s, vf%02dx\n", dest_.c_str(), fs, dest_.c_str(), ft);
}

void Vmadday(uint32_t instr)
{
	uint8_t dest = (instr >> 21) & 0xf;
	bool x = (dest >> 3) & 1;
	bool y = (dest >> 2) & 1;
	bool z = (dest >> 1) & 1;
	bool w = (dest >> 0) & 1;

	std::string dest_;
	if (x)
		dest_ += "x";
	if (y)
		dest_ += "y";
	if (z)
		dest_ += "z";
	if (w)
		dest_ += "w";
	
	int ft = (instr >> 16) & 0x1F;
	int fs = (instr >> 11) & 0x1F;

	float op = convert(vu0_state.vf[ft].yi);

	if (x)
	{
		vu0_state.acc.x = convert(vu0_state.acc.xi) + (convert(vu0_state.vf[fs].xi) * op);
	}
	if (y)
	{
		vu0_state.acc.y = convert(vu0_state.acc.yi) + (convert(vu0_state.vf[fs].yi) * op);
	}
	if (z)
	{
		vu0_state.acc.z = convert(vu0_state.acc.zi) + (convert(vu0_state.vf[fs].zi) * op);
	}
	if (w)
	{
		vu0_state.acc.w = convert(vu0_state.acc.wi) + (convert(vu0_state.vf[fs].wi) * op);
	}

	printf("vmadday.%s acc, vf%02d.%s, vf%02dy\n", dest_.c_str(), fs, dest_.c_str(), ft);
}

void Vmaddaz(uint32_t instr)
{
	uint8_t dest = (instr >> 21) & 0xf;
	bool x = (dest >> 3) & 1;
	bool y = (dest >> 2) & 1;
	bool z = (dest >> 1) & 1;
	bool w = (dest >> 0) & 1;

	std::string dest_;
	if (x)
		dest_ += "x";
	if (y)
		dest_ += "y";
	if (z)
		dest_ += "z";
	if (w)
		dest_ += "w";
	
	int ft = (instr >> 16) & 0x1F;
	int fs = (instr >> 11) & 0x1F;

	float op = convert(vu0_state.vf[ft].zi);

	if (x)
	{
		vu0_state.acc.x = convert(vu0_state.acc.xi) + (convert(vu0_state.vf[fs].xi) * op);
	}
	if (y)
	{
		vu0_state.acc.y = convert(vu0_state.acc.yi) + (convert(vu0_state.vf[fs].yi) * op);
	}
	if (z)
	{
		vu0_state.acc.z = convert(vu0_state.acc.zi) + (convert(vu0_state.vf[fs].zi) * op);
	}
	if (w)
	{
		vu0_state.acc.w = convert(vu0_state.acc.wi) + (convert(vu0_state.vf[fs].wi) * op);
	}

	printf("vmaddaz.%s acc, vf%02d.%s, vf%02dz\n", dest_.c_str(), fs, dest_.c_str(), ft);
}

void Vmulax(uint32_t instr)
{
	uint8_t dest = (instr >> 21) & 0xf;
	bool x = (dest >> 3) & 1;
	bool y = (dest >> 2) & 1;
	bool z = (dest >> 1) & 1;
	bool w = (dest >> 0) & 1;

	std::string dest_;
	if (x)
		dest_ += "x";
	if (y)
		dest_ += "y";
	if (z)
		dest_ += "z";
	if (w)
		dest_ += "w";
	
	int ft = (instr >> 16) & 0x1F;
	int fs = (instr >> 11) & 0x1F;

	float op = convert(vu0_state.vf[ft].xi);

	if (x)
	{
		vu0_state.acc.x = convert(vu0_state.vf[fs].xi) * op;
	}
	if (y)
	{
		vu0_state.acc.y = convert(vu0_state.vf[fs].yi) * op;
	}
	if (z)
	{
		vu0_state.acc.z = convert(vu0_state.vf[fs].zi) * op;
	}
	if (w)
	{
		vu0_state.acc.w = convert(vu0_state.vf[fs].wi) * op;
	}

	printf("vmulax.%s acc, vf%02d.%s, vf%02dx\n", dest_.c_str(), fs, dest_.c_str(), ft);
}

void Vmulaw(uint32_t instr)
{
	uint8_t dest = (instr >> 21) & 0xf;
	bool x = (dest >> 3) & 1;
	bool y = (dest >> 2) & 1;
	bool z = (dest >> 1) & 1;
	bool w = (dest >> 0) & 1;

	std::string dest_;
	if (x)
		dest_ += "x";
	if (y)
		dest_ += "y";
	if (z)
		dest_ += "z";
	if (w)
		dest_ += "w";
	
	int ft = (instr >> 16) & 0x1F;
	int fs = (instr >> 11) & 0x1F;

	float op = convert(vu0_state.vf[ft].wi);

	if (x)
	{
		vu0_state.acc.x = convert(vu0_state.vf[fs].xi) * op;
	}
	if (y)
	{
		vu0_state.acc.y = convert(vu0_state.vf[fs].yi) * op;
	}
	if (z)
	{
		vu0_state.acc.z = convert(vu0_state.vf[fs].zi) * op;
	}
	if (w)
	{
		vu0_state.acc.w = convert(vu0_state.vf[fs].wi) * op;
	}

	printf("vmulaw.%s acc, vf%02d.%s, vf%02dw\n", dest_.c_str(), fs, dest_.c_str(), ft);
}

void Vclipw(uint32_t instr)
{
	vu0_state.clipping.val <<= 6;
	
	int ft = (instr >> 16) & 0x1F;
	int fs = (instr >> 11) & 0x1F;

	float value = fabs(convert(vu0_state.vf[ft].wi));

	float x = convert(vu0_state.vf[fs].xi);
	float y = convert(vu0_state.vf[fs].yi);
	float z = convert(vu0_state.vf[fs].zi);

	vu0_state.clipping.val |= (x > +value);
	vu0_state.clipping.val |= (x < -value) << 1;
	vu0_state.clipping.val |= (y > +value) << 2;
	vu0_state.clipping.val |= (y > -value) << 3;
	vu0_state.clipping.val |= (z > +value) << 4;
	vu0_state.clipping.val |= (z > -value) << 5;

	printf("vclipw.xyz vf%02d.xyz, vf%02d.w\n", fs, ft);
}

void Vsqi(uint32_t instr)
{
	uint8_t dest = (instr >> 21) & 0xf;
	bool x = (dest >> 3) & 1;
	bool y = (dest >> 2) & 1;
	bool z = (dest >> 1) & 1;
	bool w = (dest >> 0) & 1;

	std::string dest_;
	if (x)
		dest_ += "x";
	if (y)
		dest_ += "y";
	if (z)
		dest_ += "z";
	if (w)
		dest_ += "w";
	
	int it = (instr >> 16) & 0x1F;
	int fs = (instr >> 11) & 0x1F;

	auto& f = vu0_state.vf[fs];

	if (x)
		WriteDataMem32(0, vu0_state.vi[it]*16+0x0, f.x);
	if (y)
		WriteDataMem32(0, vu0_state.vi[it]*16+0x4, f.y);
	if (z)
		WriteDataMem32(0, vu0_state.vi[it]*16+0x8, f.z);
	if (w)
		WriteDataMem32(0, vu0_state.vi[it]*16+0xc, f.w);
	
	VU0::vu0_state.vi[it]++;

	printf("sqi.%s vf%02d, (vi%02d++).%s\n", dest_.c_str(), fs, it, dest_.c_str());
}

void Viswr(uint32_t instr)
{
	uint8_t dest = (instr >> 21) & 0xf;
	bool x = (dest >> 3) & 1;
	bool y = (dest >> 2) & 1;
	bool z = (dest >> 1) & 1;
	bool w = (dest >> 0) & 1;

	std::string dest_;
	if (x)
		dest_ += "x";
	if (y)
		dest_ += "y";
	if (z)
		dest_ += "z";
	if (w)
		dest_ += "w";

	// Make sure only one dest bit is set
	assert((x && !y && !z && !w) || (!x && y && !z && !w) || (!x && !y && z && !w) || (!x && !y && !z && w));

	int it = (instr >> 16) & 0x1F;
	int is = (instr >> 11) & 0x1F;

	printf("viswr.%s vi%02d, (vi%02d).%s\n", dest_.c_str(), it, is, dest_.c_str());
	
	uint16_t addr = vu0_state.vi[is];

	if (y)
		addr += 4;
	if (z)
		addr += 8;
	if (w)
		addr += 12;
	
	WriteDataMem32(0, addr, vu0_state.vi[it]);
}
}
}