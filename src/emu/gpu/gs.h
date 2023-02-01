// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstdint>

namespace GS
{

void WriteGSCSR(uint64_t data);
inline void WriteGSSMODE1(uint64_t data) {}
inline void WriteGSSMODE2(uint64_t data) {}
inline void WriteGSSRFSH(uint64_t data) {}
inline void WriteGSSYNCH1(uint64_t data) {}
inline void WriteGSSYNCH2(uint64_t data) {}
inline void WriteGSSYNCV(uint64_t data) {}

}  // namespace GS
