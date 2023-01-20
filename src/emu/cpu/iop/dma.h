#pragma once

#include <cstdint>

namespace IopDma
{

void WriteDPCR(uint32_t data);
void WriteDPCR2(uint32_t data);
void WriteDMACEN(uint32_t data);
void WriteDICR(uint32_t data);
void WriteDICR2(uint32_t data);

void WriteChannel(uint32_t addr, uint32_t data);
void WriteNewChannel(uint32_t addr, uint32_t data);

uint32_t ReadDMACEN();
uint32_t ReadDICR2();
uint32_t ReadDICR();
uint32_t ReadDPCR2();
uint32_t ReadDPCR();

}