#pragma once

#include <cstdint>
#include <util/uint128.h>

namespace VIF
{

void WriteFBRST(int vif_num, uint32_t data);
void WriteMASK(int vif_num, uint32_t data);

void WriteVIF1FIFO(uint128_t data);
void WriteVIF0FIFO(uint128_t data);

}