// (c) Copyright 2022 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstdint>
#include <emu/cpu/ee/x64/reg_alloc.h>
#include <3rdparty/xbyak/xbyak.h>

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
	void EmitSLTI(IRInstruction i);
	void EmitBC(IRInstruction i);
	void EmitIncPC(IRInstruction i);
	void EmitOR(IRInstruction i);
	void EmitJA(IRInstruction i);
	void EmitJumpImm(IRInstruction i);
	void EmitAdd(IRInstruction i);
	void EmitMemoryStore(IRInstruction i);
	void EmitIR(IRInstruction i);
public:
	Emitter();
	void Dump();
	void TranslateBlock(Block* block);

	uint8_t* GetFreeBase();
};

extern Emitter* emit;

}