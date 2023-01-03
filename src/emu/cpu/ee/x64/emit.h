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
	RegisterAllocator* reg_alloc;

	void EmitSaveHostRegs();
	void EmitMov(IRInstruction i);
	void EmitIR(IRInstruction i);
public:
	Emitter();
	void Dump();
	void TranslateBlock(Block* block);
};

extern Emitter* emit;

}