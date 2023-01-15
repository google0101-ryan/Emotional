// (c) Copyright 2022 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

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
	IncPC = 6,
	OR = 7,
	JumpAlways = 8,
	Add = 9,
	MemoryStore = 10,
	JumpImm = 11, // We have to do some fancy or'ing for JAL, J so it can't share an op with JR, JALR
	AND = 12,
	Shift = 13,
	Mult = 14,
	Div = 15,
	UhOh = 16,
	MemoryLoad = 17,
	MoveFromLo = 18,
	BranchRegImm = 19,
	MoveFromHi = 20,
	Sub = 21,
	MovCond = 22,
	Div1 = 23,
};

struct IRValue
{
public:
	enum Type
	{
		Imm,
		Reg,
		Cop0Reg,
		Float
	};
private:
	union
	{
		uint64_t imm;
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
	bool IsReg() {return type == Reg;}
	bool IsFloat() {return type == Float;}
	// Can be used for guest and COP0 registers
	void SetReg(uint32_t reg) {value.register_num = reg;}
	void SetImm(uint16_t imm) {value.imm = (int32_t)(int16_t)imm;}
	void SetImm32(uint32_t imm) {value.imm = imm;}
	void SetImmUnsigned(uint16_t imm) {value.imm = (uint32_t)imm;}
	void SetImm32Unsigned(uint32_t imm) {value.imm = imm;}
	

	uint32_t GetImm() {return value.imm;}
	uint32_t GetReg() {return value.register_num;}
};

struct IRInstruction
{
	uint8_t instr;
	// Arguments are left -> right
	std::vector<IRValue> args;

	bool should_link = false;
	bool is_logical = false;
	bool is_unsigned = false;
	bool is_likely = false;
	bool is_mmi_divmul = false;

	enum Direction
	{
		Left,
		Right
	} direction;
	
	enum BranchType
	{
		NE = 0,
		EQ = 1,
		GE = 2,
		LE = 3,
		GT = 4,
		LT = 5,
	} b_type;

	enum class MovCond : int
	{
		N = 1,
		Z = 2,
	} mov_cond;

	enum AccessSize
	{
		U64 = 0,
		U32 = 1,
		U16 = 2,
		U8 = 3,
		S8 = 4,
	} access_size;

	enum InstrSize // ADDIU vs DADDIU, etc
	{
		Size64,
		Size32
	} size = Size32;

	enum ShiftType
	{
		LeftLogical,
	} shift_type;

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
	uint8_t* entry;
	std::vector<IRInstruction> ir;
	size_t cycles;
};

class JIT
{
private:
	Block* cur_block;
	std::vector<Block*> blockCache;

	void EmitJ(uint32_t instr, EE_JIT::IRInstruction& i); // 0x02
	void EmitJAL(uint32_t instr, EE_JIT::IRInstruction& i); // 0x03
	void EmitBEQ(uint32_t instr, EE_JIT::IRInstruction& i); // 0x04
	void EmitBNE(uint32_t instr, EE_JIT::IRInstruction& i);  // 0x05
	void EmitBLEZ(uint32_t instr, EE_JIT::IRInstruction& i); // 0x06
	void EmitBGTZ(uint32_t instr, EE_JIT::IRInstruction& i); // 0x06
	void EmitADDIU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x09
	void EmitSLTI(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0A
	void EmitSLTIU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0B
	void EmitANDI(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0C
	void EmitORI(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0D
	void EmitLUI(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0F
	void EmitCOP0(uint32_t instr, EE_JIT::IRInstruction& i); // 0x10
	void EmitBEQL(uint32_t instr, EE_JIT::IRInstruction& i); // 0x14
	void EmitBNEL(uint32_t instr, EE_JIT::IRInstruction& i); // 0x15
	void EmitLB(uint32_t instr, EE_JIT::IRInstruction& i); // 0x20
	void EmitLW(uint32_t instr, EE_JIT::IRInstruction& i); // 0x23
	void EmitLBU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x24
	void EmitLHU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x25
	void EmitSB(uint32_t instr, EE_JIT::IRInstruction& i); // 0x28
	void EmitSH(uint32_t instr, EE_JIT::IRInstruction& i); // 0x29
	void EmitSW(uint32_t instr, EE_JIT::IRInstruction& i); // 0x2B
	void EmitLD(uint32_t instr, EE_JIT::IRInstruction& i); // 0x37
	void EmitSWC1(uint32_t instr, EE_JIT::IRInstruction& i); // 0x39
	void EmitSD(uint32_t instr, EE_JIT::IRInstruction& i); // 0x3f

	void EmitSLL(uint32_t instr, EE_JIT::IRInstruction& i); // 0x00
	void EmitSRL(uint32_t instr, EE_JIT::IRInstruction& i); // 0x02
	void EmitSRA(uint32_t instr, EE_JIT::IRInstruction& i); // 0x02
	void EmitJR(uint32_t instr, EE_JIT::IRInstruction& i); // 0x08
	void EmitJALR(uint32_t instr, EE_JIT::IRInstruction& i); // 0x09
	void EmitMOVN(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0B
	void EmitBreak(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0D
	void EmitMFHI(uint32_t instr, EE_JIT::IRInstruction& i); // 0x10
	void EmitMFLO(uint32_t instr, EE_JIT::IRInstruction& i); // 0x12
	void EmitMULT(uint32_t instr, EE_JIT::IRInstruction& i); // 0x18
	void EmitDIV(uint32_t instr, EE_JIT::IRInstruction& i); // 0x1a
	void EmitDIVU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x1b
	void EmitADDU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x21
	void EmitSUBU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x23
	void EmitAND(uint32_t instr, EE_JIT::IRInstruction& i); // 0x24
	void EmitOR(uint32_t instr, EE_JIT::IRInstruction& i); // 0x25
	void EmitSLT(uint32_t instr, EE_JIT::IRInstruction& i); // 0x2A
	void EmitSLTU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x2B
	void EmitDADDU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x2D
	
	void EmitBLTZ(uint32_t instr, EE_JIT::IRInstruction& i); // 0x00
	void EmitBGEZ(uint32_t instr, EE_JIT::IRInstruction& i); // 0x01
	
	void EmitMFLO1(uint32_t instr, EE_JIT::IRInstruction& i); // 0x12
	void EmitDIVU1(uint32_t instr, EE_JIT::IRInstruction& i); // 0x1B

	void EmitMFC0(uint32_t instr, EE_JIT::IRInstruction& i); // 0x00
	void EmitMTC0(uint32_t instr, EE_JIT::IRInstruction& i); // 0x04
	void EmitIncPC(EE_JIT::IRInstruction& i);
public:
	using EntryFunc = void(*)();

	void EmitIR(uint32_t instr);

	void EmitPrequel(uint32_t guest_start);
	EntryFunc EmitDone(size_t cycles_taken);
	Block* GetExistingBlock(uint32_t start);

	bool DoesBlockExist(uint32_t addr);
};

}

namespace EmotionEngine
{

struct ProcessorState
{
	uint128_t regs[32];
	uint32_t cop0_regs[32];
	uint32_t pc, next_pc;
	uint32_t hi1, hi;
	uint32_t lo1, lo;
	union FPR
	{
		uint32_t i;
		int32_t s;
		float f;
	} fprs[32];

	uint32_t pc_at;
};

void Reset();
int Clock();
void Dump();
ProcessorState* GetState();

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