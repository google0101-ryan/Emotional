#pragma once

#include <cstdint>
#include <util/uint128.h>

namespace GIF
{

void Reset();
void WriteCtrl32(uint32_t data);

uint32_t ReadStat();

void WriteFIFO(uint128_t data);

}