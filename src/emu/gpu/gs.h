// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstdint>

namespace GS
{

void Initialize();
void DumpVram();

void SetVblankStart(bool start);
void SetHblank(bool hb);
void UpdateOddFrame();

void WriteGSCSR(uint64_t data);
uint64_t ReadGSCSR();

void WriteIMR(uint64_t data);

bool VSIntEnabled();
bool HSIntEnabled();

void WritePRIM(uint64_t data);

void UpdateFPS(double fps);

void WriteBGCOLOR(uint64_t data);
void WriteDISPFB1(uint64_t data);
void WriteDISPLAY1(uint64_t data);
void WriteDISPFB2(uint64_t data);
void WriteDISPLAY2(uint64_t data);

// Used by the GIF to send data
void WriteRegister(uint64_t reg, uint64_t data);
void WriteHWReg(uint64_t data);
void WriteRGBAQ(uint8_t r, uint8_t g, uint8_t b, uint8_t a, float q = 1.0f);
void WriteXYZF(int32_t x, int32_t y, int32_t z, uint8_t f, bool adc);

inline void WriteGSSMODE1(uint64_t data) {}
void WriteGSSMODE2(uint64_t data);
inline void WriteGSSRFSH(uint64_t data) {}
inline void WriteGSSYNCH1(uint64_t data) {}
inline void WriteGSSYNCH2(uint64_t data) {}
inline void WriteGSSYNCV(uint64_t data) {}
inline void WriteGSPMODE(uint64_t data) {}

}  // namespace GS
