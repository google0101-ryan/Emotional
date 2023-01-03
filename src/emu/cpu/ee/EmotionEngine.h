#pragma once

#include <cstdint>
#include <vector>
#include <util/uint128.h>

namespace EE_JIT
{

enum IRInstrs
{
	MOV = 0,
	NOP = 1,
	SLTI = 2,
	SaveHostRegs = 3,
	RestoreHostRegs = 4,
	BranchConditional = 5,
};

struct IRValue
{
public:
	enum Type
	{
		Imm,
		Reg,
		Cop0Reg,
		FP
	};
private:
	union
	{
		uint32_t imm;
		int register_num;
		float fp_value;
		int cop_regnum;
	} value;

	Type type;
public:
	IRValue(Type type)
	: type(type) {}

	bool IsImm() {return type == Imm;}
	bool IsCop0() {return type == Cop0Reg;}
	// Can be used for guest and COP0 registers
	void SetReg(uint32_t reg) {value.register_num = reg;}
	void SetImm(uint16_t imm) {value.imm = (int32_t)(int16_t)imm;}

	uint32_t GetImm() {return value.imm;}
	uint32_t GetReg() {return value.register_num;}
};

struct IRInstruction
{
	uint8_t instr;
	// Arguments are right -> left
	std::vector<IRValue> args;
	
	enum BranchType
	{
		NE = 0,
	} b_type;

	static IRInstruction Build(std::vector<IRValue> args, uint8_t i_type)
	{
		IRInstruction i;
		i.instr = i_type;
		i.args = args;

		return i;
	}
};

struct Block
{
	uint32_t guest_start;
	std::vector<IRInstruction> ir;
};

class JIT
{
private:

	Block* cur_block;
	std::vector<Block*> blockCache;

	void EmitBNE(uint32_t instr, EE_JIT::IRInstruction& i);  // 0x05
	void EmitSLTI(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0A
	void EmitCOP0(uint32_t instr, EE_JIT::IRInstruction& i); // 0x10


	void EmitMFC0(uint32_t instr, EE_JIT::IRInstruction& i);
public:
	void EmitIR(uint32_t instr);

	void EmitPrequel();
	void EmitDone();
};

}

namespace EmotionEngine
{

struct ProcessorState
{
	uint128_t regs[32];
	uint32_t cop0_regs[32];
	uint32_t pc, next_pc;
};

void Reset();
void Clock();
void Dump();
ProcessorState* GetState();

}