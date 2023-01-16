#pragma once

#include <cstdint>
#include <util/uint128.h>

namespace VectorUnit
{

void WriteDataMem128(int vector, uint32_t addr, uint128_t data);

void WriteCodeMem128(int vector, uint32_t addr, uint128_t data);
void WriteCodeMem64(int vector, uint32_t addr, uint64_t data);

void Dump();

namespace VU0
{

// VU0 specific stuff, like COP2 opcodes

}

}