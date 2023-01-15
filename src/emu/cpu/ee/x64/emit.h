// (c) Copyright 2022 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstdint>
#include <emu/cpu/ee/x64/reg_alloc.h>
#include <3rdparty/xbyak/xbyak.h>
#include <vector>

namespace EE_JIT
{

struct Block;
struct IRInstruction;

class Emitter
{
private:
	Xbyak::CodeGenerator* cg;
	uint8_t* base;
	uint8_t* free_base;
	RegisterAllocator* reg_alloc;
	size_t sizeWithoutCurBlock = 0;

	void EmitSaveHostRegs();
	void EmitRestoreHostRegs();
	void EmitMov(IRInstruction i);
	void EmitMovCond(IRInstruction i);
	void EmitSLTI(IRInstruction i);
	void EmitBC(IRInstruction i);
	void EmitIncPC(IRInstruction i);
	void EmitOR(IRInstruction i);
	void EmitAND(IRInstruction i);
	void EmitJA(IRInstruction i);
	void EmitJumpImm(IRInstruction i);
	void EmitAdd(IRInstruction i);
	void EmitSub(IRInstruction i);
	void EmitMemoryStore(IRInstruction i);
	void EmitMemoryLoad(IRInstruction i);
	void EmitShift(IRInstruction i);
	void EmitMULT(IRInstruction i);
	void EmitDIV(IRInstruction i);
	void EmitUhOh(IRInstruction i);
	void EmitIR(IRInstruction i);
	void EmitMoveFromLo(IRInstruction i);
	void EmitMoveFromHi(IRInstruction i);
	void EmitBranchRegImm(IRInstruction i);
public:
	Emitter();
	void Dump();
	void TranslateBlock(Block* block);

	const uint8_t* GetFreeBase();
};

extern Emitter* emit;

}