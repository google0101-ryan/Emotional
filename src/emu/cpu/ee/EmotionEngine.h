#pragma once

#include <cstdint>
#include <vector>
#include <util/uint128.h>

namespace EE_JIT
{

struct IRInstruction;

class JIT
{
private:
	struct Block
	{
		uint32_t guest_start;
		std::vector<IRInstruction> ir;
	};

	Block* cur_block;

	void EmitSLTI(uint32_t instr, EE_JIT::IRInstruction& i);
	void EmitCOP0(uint32_t instr, EE_JIT::IRInstruction& i);


	void EmitMFC0(uint32_t instr, EE_JIT::IRInstruction& i);
public:
	void EmitIR(uint32_t instr);
};

}

namespace EmotionEngine
{

struct ProcessorState
{
	uint128_t regs[32];
	uint32_t pc, next_pc;
};

void Reset();
void Clock();

}