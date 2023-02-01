// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <util/uint128.h>

#include <cstdint>

namespace GIF
{

void Reset();
void WriteCtrl32(uint32_t data);

uint32_t ReadStat();

void WriteFIFO(uint128_t data);

}  // namespace GIF
