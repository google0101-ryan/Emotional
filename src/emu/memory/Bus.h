// (c) Copyright 2022 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstdint>

namespace Bus
{

void LoadBios(uint8_t* data);

uint32_t Read32(uint32_t addr);
void Write64(uint32_t addr, uint64_t data);
void Write32(uint32_t addr, uint32_t data);

}