#include "vu.h"

namespace VectorUnit
{

uint32_t data_address_mask[2] = {0xfff, 0x3fff};
uint32_t code_address_mask[2] = {0xfff, 0x3fff};

uint8_t data[2][0x4000];
uint8_t code[2][0x4000];

void WriteDataMem128(int vector, uint32_t addr, uint128_t data)
{
    if (vector == 1)
        addr = (addr - 0x1100C000);
    else
        addr = (addr - 0x11004000);
    
    addr &= data_address_mask[vector];

    *(uint128_t*)&code[vector][addr] = data;
}

}