#pragma once

#include <cstdint>
#include <util/uint128.h>

namespace VectorUnit
{

void WriteDataMem128(int vector, uint32_t addr, uint128_t data);

}