// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <util/uint128.h>
#include <3party/robin_hood.h>

namespace EmotionEngine
{

struct ProcessorState
{
	uint128_t regs[32];
	uint32_t cop0_regs[32];
	union
	{
		float f;
		uint32_t u;
		int32_t i;
	} acc;
	uint32_t pc, next_pc;
	uint64_t hi1, hi;
	uint64_t lo1, lo;
	union FPR
	{
		uint32_t i;
		int32_t s;
		float f;
	} fprs[32];
	bool c = false;

	uint32_t pc_at;
};

extern bool can_disassemble;

void Reset();
int Clock(int cycles);
void Dump();
ProcessorState* GetState();
void MarkDirty(uint32_t address, uint32_t size);
void EmitPrequel();
void Exception(uint8_t code);

void SetIp1Pending(); // Set IP1 to pending, signalling a DMA interrupt
void ClearIp1Pending(); // Clear a DMA interrupt

void SetIp0Pending();

extern bool can_dump;

void CheckCacheFull();
bool DoesBlockExit(uint32_t addr);
void EmitIR(uint32_t instr);
bool IsBranch(uint32_t instr);
uint64_t GetExistingBlockCycles(uint32_t addr);

inline const char* Reg(int index)
    {
        switch (index)
        {
        case 0:
            return "$zero";
        case 1:
            return "$at";
        case 2:
            return "$v0";
        case 3:
            return "$v1";
        case 4:
            return "$a0";
        case 5:
            return "$a1";
        case 6:
            return "$a2";
        case 7:
            return "$a3";
        case 8:
            return "$t0";
        case 9:
            return "$t1";
        case 10:
            return "$t2";
        case 11:
            return "$t3";
        case 12:
            return "$t4";
        case 13:
            return "$t5";
        case 14:
            return "$t6";
        case 15:
            return "$t7";
        case 16:
            return "$s0";
        case 17:
            return "$s1";
        case 18:
            return "$s2";
        case 19:
            return "$s3";
        case 20:
            return "$s4";
        case 21:
            return "$s5";
        case 22:
            return "$s6";
        case 23:
            return "$s7";
        case 24:
            return "$t8";
        case 25:
            return "$t9";
        case 26:
            return "$k0";
        case 27:
            return "$k1";
        case 28:
            return "$gp";
        case 29:
            return "$sp";
        case 30:
            return "$fp";
        case 31:
            return "$ra";
        default:
            return "";
        }
    }

}