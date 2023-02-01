// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <util/uint128.h>

namespace SIF
{

void WriteMSCOM_EE(uint32_t data);
void WriteMSFLG_EE(uint32_t data);
void WriteSMFLG_EE(uint32_t data);
void WriteCTRL_EE(uint32_t data);
void WriteBD6_EE(uint32_t data);

void WriteCTRL_IOP(uint32_t data);
void WriteSMCOM_IOP(uint32_t data);
void WriteSMFLG_IOP(uint32_t data);
void WriteMSFLG_IOP(uint32_t data);

uint32_t ReadMSCOM_EE();
uint32_t ReadSMCOM();
uint32_t ReadMSFLG();
uint32_t ReadSMFLG();
uint32_t ReadCTRL();

size_t FIFO0_size();
size_t FIFO1_size();

void WriteFIFO0(uint32_t data);
void WriteFIFO1(uint32_t data);

uint32_t ReadAndPopSIF1();

}  // namespace SIF
