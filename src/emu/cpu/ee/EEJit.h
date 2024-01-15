#pragma once

#include <cstdint>
#include <vector>

enum IRInstrs
{
    NOP,
    PROLOGUE,
    EPILOGUE,
    MOVE, // General purpose move (COP0, r<-imm, etc)
    SLT, // Compare and set register (can be immediate or reg, signed or unsigned)
    BRANCH, // PC-Relative branch on condition = true
    OR, // Do a logical or between a register and an immediate or two registers
    JUMP, // Either a jump to an imm, or else a jump to a register
	ADD, // Add two pieces of data (reg+imm, reg+reg)
	STORE, // Store memory op
	AND, // Bitwise AND
	SHIFT, // Shift logical, arithmetic, left, right, etc...
	MULT, // Multiply
};

struct IRValue
{
public:
	enum Type
	{
		Imm,
		Reg,
		Cop0Reg,
		Cop1Reg,
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
public:
	Type type;
public:
	IRValue(Type type)
	: type(type) {}

	bool IsImm() {return type == Imm;}
	bool IsCop0() {return type == Cop0Reg;}
	bool IsCop1() {return type == Cop1Reg;}
	bool IsReg() {return type == Reg;}
	bool IsFloat() {return type == Float;}
	// Can be used for guest, COP0, and COP1 registers
	void SetReg(uint32_t reg) {value.register_num = reg;}
	void SetImm(uint16_t imm) {value.imm = (int32_t)(int16_t)imm;}
	void SetImm64(uint16_t imm) {value.imm = (int64_t)(int16_t)imm;}
	void SetImm64From32(uint32_t imm) {value.imm = (int64_t)(int32_t)imm;}
	void SetImm32(uint32_t imm) {value.imm = imm;}
	void SetImmUnsigned(uint16_t imm) {value.imm = (uint32_t)imm;}
	void SetImm32Unsigned(uint32_t imm) {value.imm = imm;}
	

	uint32_t GetImm() {return value.imm;}
	uint64_t GetImm64() {return value.imm;}
	uint32_t GetReg() {return value.register_num;}
};

struct IRInstruction
{
	// The IR instruction
	uint8_t instr;
	// Arguments are left -> right
	std::vector<IRValue> args;

	bool should_link = false;
	bool is_logical = false;
	bool is_unsigned = false;
	bool is_likely = false;
	bool is_mmi_divmul = false;

	// Shift direction
	enum Direction
	{
		Left,
		Right
	} direction;
	
	// Branch condition
	enum BranchType
	{
		NE = 0,
		EQ = 1,
		GE = 2,
		LE = 3,
		GT = 4,
		LT = 5,
        AL = 6,
	} b_type;

	// MOVN, MOVZ
	enum class MovCond : int
	{
		N = 1,
		Z = 2,
	} mov_cond;

	// Memory access sizes
	enum AccessSize
	{
		U64 = 0,
		U32 = 1,
		U16 = 2,
		U8 = 3,
		U128 = 4,
	} access_size;

	enum InstrSize // ADDIU vs DADDIU, etc
	{
		Size64,
		Size32
	} size = Size32;

	static IRInstruction Build(std::vector<IRValue> args, uint8_t i_type)
	{
		IRInstruction i;
		i.instr = i_type;
		i.args = args;

		return i;
	}

	uint32_t opcode;
};

typedef void (*blockEntry)(void* statePtr, uint32_t blockPC);

struct Block
{
    uint32_t addr, cycles;
    std::vector<IRInstruction> instructions;
    blockEntry entryPoint;
};

namespace EEJit
{

int Clock(int cycles);

void Initialize();
void Dump();

}