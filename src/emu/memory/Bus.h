// (c) Copyright 2022 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstdint>

namespace Bus
{

void LoadBios(uint8_t* data);

void Dump();

uint64_t Read64(uint32_t addr);
uint32_t Read32(uint32_t addr);
uint16_t Read16(uint32_t addr);
uint8_t Read8(uint32_t addr);

void Write64(uint32_t addr, uint64_t data);
void Write32(uint32_t addr, uint32_t data);
void Write16(uint32_t addr, uint16_t data);
void Write8(uint32_t addr, uint8_t data);

}