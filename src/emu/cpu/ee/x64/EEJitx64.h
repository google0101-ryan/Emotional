#pragma once

#include "GuestRegister.h"
#include <cstdint>

struct Block;

namespace EEJitX64
{

void JitStoreReg(GuestRegister reg);
void JitLoadReg(GuestRegister reg, int hostReg);

// Translate a JIT block from IR to host code
// Modifies the `entry` pointer in the block
void TranslateBlock(Block* block);
void CacheBlock(Block* block);
Block* GetBlockForAddr(uint32_t addr);

void Initialize();

void Dump();

}