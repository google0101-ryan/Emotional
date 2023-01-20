// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstdint>

namespace DMAC
{

void WriteVIF0Channel(uint32_t addr, uint32_t data);
void WriteVIF1Channel(uint32_t addr, uint32_t data);
void WriteGIFChannel(uint32_t addr, uint32_t data);
void WriteIPUFROMChannel(uint32_t addr, uint32_t data);
void WriteIPUTOChannel(uint32_t addr, uint32_t data);
void WriteSIF0Channel(uint32_t addr, uint32_t data);
void WriteSIF1Channel(uint32_t addr, uint32_t data);
void WriteSIF2Channel(uint32_t addr, uint32_t data);
void WriteSPRFROMChannel(uint32_t addr, uint32_t data);
void WriteSPRTOChannel(uint32_t addr, uint32_t data);

uint32_t ReadSIF0Channel(uint32_t addr);

void WriteDSTAT(uint32_t data);
uint32_t ReadDSTAT();

void WriteDCTRL(uint32_t data);
uint32_t ReadDCTRL();

void WriteDPCR(uint32_t data);
uint32_t ReadDPCR();

void WriteSQWC(uint32_t data);

}