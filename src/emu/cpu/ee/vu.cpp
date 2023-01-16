#include "vu.h"
#include <fstream>

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

    *(uint128_t*)&data[vector][addr] = _data;
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
	out_file.open("vu1_code.bin");
	for (int i = 0; i < 0x4000; i++)
	{
		out_file << code[1][i];
	}
}
}