// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <util/uint128.h>
#include <3party/robin_hood.h>

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
	Shift64 = 24,
	XOR = 25,
	UpdateCopCount = 26,
	POR = 27,
	NOR = 28,
	LDL = 29,
	LDR = 30,
	SDL = 31,
	SDR = 32,
	PADDUSW = 33,
	DI = 34,
	ERET = 35,
	SYSCALL = 36,
	EI = 37,
	PLZCW = 38,
	PMFHI = 39,
	PMFLO = 40,
	MTHI = 41,
	MTLO = 42,
	PCPYLD = 43,
	PSUBB = 44,
	PNOR = 45,
	PAND = 46,
	PCPYUD = 47,
	BC0T = 48,
	BC0F = 49,
	PCPYH = 50,
	CFC2 = 51,
	CTC2 = 52,
	VISWR = 53,
	QMFC2 = 54,
	QMTC2 = 55,
	VSUB = 56,
	VSQI = 57,
	VIADD = 58,
	ADDAS = 59,
	PADDSB = 60,
	MADD = 61,
	MADDS = 62,
	CVTS = 63,
	MULS = 64,
	CVTW = 65,
	DIVS = 66,
	MOVS = 67,
	ADDS = 68,
	SUBS = 69,
	NEGS = 70,
	LQC2 = 71,
	VMULAX = 72,
	VMADDAZ = 73,
	VMADDAY = 74,
	VMADDW = 75,
	SQC2 = 76,
	VMADDZ = 77,
	VMADDAX = 78,
	VMULAW = 79,
	VCLIPW = 80,
	CLES = 81,
	BC1F = 82,
	CEQS = 83,
	BC1T = 84,
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
	void SetImm32(uint32_t imm) {value.imm = imm;}
	void SetImmUnsigned(uint16_t imm) {value.imm = (uint32_t)imm;}
	void SetImm32Unsigned(uint32_t imm) {value.imm = imm;}
	

	uint32_t GetImm() {return value.imm;}
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

struct Block
{
	uint8_t* entry;
	uint32_t guest_addr;
	std::vector<IRInstruction> ir;
	size_t cycles;
};

class JIT
{
private:
	Block* cur_block;
	robin_hood::unordered_flat_map<uint32_t, Block*> blockCache;

	void EmitJ(uint32_t instr, EE_JIT::IRInstruction& i); // 0x02
	void EmitJAL(uint32_t instr, EE_JIT::IRInstruction& i); // 0x03
	void EmitBEQ(uint32_t instr, EE_JIT::IRInstruction& i); // 0x04
	void EmitBNE(uint32_t instr, EE_JIT::IRInstruction& i);  // 0x05
	void EmitBLEZ(uint32_t instr, EE_JIT::IRInstruction& i); // 0x06
	void EmitBGTZ(uint32_t instr, EE_JIT::IRInstruction& i); // 0x06
	void EmitADDI(uint32_t instr, EE_JIT::IRInstruction& i); // 0x08
	void EmitADDIU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x09
	void EmitSLTI(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0A
	void EmitSLTIU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0B
	void EmitANDI(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0C
	void EmitORI(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0D
	void EmitXORI(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0E
	void EmitLUI(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0F
	void EmitCOP0(uint32_t instr, EE_JIT::IRInstruction& i); // 0x10
	void EmitBEQL(uint32_t instr, EE_JIT::IRInstruction& i); // 0x14
	void EmitBNEL(uint32_t instr, EE_JIT::IRInstruction& i); // 0x15
	void EmitDADDIU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x19
	void EmitLDL(uint32_t instr, EE_JIT::IRInstruction& i); // 0x1a
	void EmitLDR(uint32_t instr, EE_JIT::IRInstruction& i); // 0x1b
	void EmitLQ(uint32_t instr, EE_JIT::IRInstruction& i); // 0x1f
	void EmitSQ(uint32_t instr, EE_JIT::IRInstruction& i); // 0x1f
	void EmitLB(uint32_t instr, EE_JIT::IRInstruction& i); // 0x20
	void EmitLH(uint32_t instr, EE_JIT::IRInstruction& i); // 0x21
	void EmitLW(uint32_t instr, EE_JIT::IRInstruction& i); // 0x23
	void EmitLBU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x24
	void EmitLHU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x25
	void EmitLWU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x27
	void EmitSB(uint32_t instr, EE_JIT::IRInstruction& i); // 0x28
	void EmitSH(uint32_t instr, EE_JIT::IRInstruction& i); // 0x29
	void EmitSW(uint32_t instr, EE_JIT::IRInstruction& i); // 0x2B
	void EmitSDL(uint32_t instr, EE_JIT::IRInstruction& i); // 0x2C
	void EmitSDR(uint32_t instr, EE_JIT::IRInstruction& i); // 0x2D
	void EmitLWC1(uint32_t instr, EE_JIT::IRInstruction& i); // 0x31
	void EmitLQC2(uint32_t instr, EE_JIT::IRInstruction& i); // 0x36
	void EmitLD(uint32_t instr, EE_JIT::IRInstruction& i); // 0x37
	void EmitSWC1(uint32_t instr, EE_JIT::IRInstruction& i); // 0x39
	void EmitSQC2(uint32_t instr, EE_JIT::IRInstruction& i); // 0x3e
	void EmitSD(uint32_t instr, EE_JIT::IRInstruction& i); // 0x3f

	void EmitSLL(uint32_t instr, EE_JIT::IRInstruction& i); // 0x00
	void EmitSRL(uint32_t instr, EE_JIT::IRInstruction& i); // 0x02
	void EmitSRA(uint32_t instr, EE_JIT::IRInstruction& i); // 0x03
	void EmitSLLV(uint32_t instr, EE_JIT::IRInstruction& i); // 0x04
	void EmitSRLV(uint32_t instr, EE_JIT::IRInstruction& i); // 0x06
	void EmitSRAV(uint32_t instr, EE_JIT::IRInstruction& i); // 0x07
	void EmitJR(uint32_t instr, EE_JIT::IRInstruction& i); // 0x08
	void EmitJALR(uint32_t instr, EE_JIT::IRInstruction& i); // 0x09
	void EmitMOVZ(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0A
	void EmitMOVN(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0B
	void EmitSyscall(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0C
	void EmitBreak(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0D
	void EmitMFHI(uint32_t instr, EE_JIT::IRInstruction& i); // 0x10
	void EmitMTHI(uint32_t instr, EE_JIT::IRInstruction& i); // 0x11
	void EmitMFLO(uint32_t instr, EE_JIT::IRInstruction& i); // 0x12
	void EmitMTLO(uint32_t instr, EE_JIT::IRInstruction& i); // 0x13
	void EmitDSLLV(uint32_t instr, EE_JIT::IRInstruction& i); // 0x14
	void EmitDSRLV(uint32_t instr, EE_JIT::IRInstruction& i); // 0x16
	void EmitDSRAV(uint32_t instr, EE_JIT::IRInstruction& i); // 0x17
	void EmitMULT(uint32_t instr, EE_JIT::IRInstruction& i); // 0x18
	void EmitMULTU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x19
	void EmitDIV(uint32_t instr, EE_JIT::IRInstruction& i); // 0x1a
	void EmitDIVU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x1b
	void EmitADD(uint32_t instr, EE_JIT::IRInstruction& i); // 0x20
	void EmitADDU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x21
	void EmitSUB(uint32_t instr, EE_JIT::IRInstruction& i); // 0x22
	void EmitSUBU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x23
	void EmitAND(uint32_t instr, EE_JIT::IRInstruction& i); // 0x24
	void EmitOR(uint32_t instr, EE_JIT::IRInstruction& i); // 0x25
	void EmitXOR(uint32_t instr, EE_JIT::IRInstruction& i); // 0x25
	void EmitNOR(uint32_t instr, EE_JIT::IRInstruction& i); // 0x27
	void EmitSLT(uint32_t instr, EE_JIT::IRInstruction& i); // 0x2A
	void EmitSLTU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x2B
	void EmitDADDU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x2D
	void EmitDSUBU(uint32_t instr, EE_JIT::IRInstruction& i); // 0x2F
	void EmitDSLL(uint32_t instr, EE_JIT::IRInstruction& i); // 0x38
	void EmitDSRL(uint32_t instr, EE_JIT::IRInstruction& i); // 0x3A
	void EmitDSLL32(uint32_t instr, EE_JIT::IRInstruction& i); // 0x3C
	void EmitDSRL32(uint32_t instr, EE_JIT::IRInstruction& i); // 0x3E
	void EmitDSRA32(uint32_t instr, EE_JIT::IRInstruction& i); // 0x3F
	
	void EmitBLTZ(uint32_t instr, EE_JIT::IRInstruction& i); // 0x00
	void EmitBGEZ(uint32_t instr, EE_JIT::IRInstruction& i); // 0x01
	void EmitBLTZL(uint32_t instr, EE_JIT::IRInstruction& i); // 0x02
	void EmitBGEZL(uint32_t instr, EE_JIT::IRInstruction& i); // 0x03
	void EmitBGEZAL(uint32_t instr, EE_JIT::IRInstruction& i); // 0x11
	
	// MMI
	void EmitMADD(uint32_t instr, EE_JIT::IRInstruction& i); // 0x00
	void EmitPLZCW(uint32_t instr, EE_JIT::IRInstruction& i); // 0x04
	void EmitMFHI1(uint32_t instr, EE_JIT::IRInstruction& i); // 0x10
	void EmitMTHI1(uint32_t instr, EE_JIT::IRInstruction& i); // 0x11
	void EmitMFLO1(uint32_t instr, EE_JIT::IRInstruction& i); // 0x12
	void EmitMTLO1(uint32_t instr, EE_JIT::IRInstruction& i); // 0x13
	void EmitMULT1(uint32_t instr, EE_JIT::IRInstruction& i); // 0x18
	void EmitDIVU1(uint32_t instr, EE_JIT::IRInstruction& i); // 0x1B
	void EmitPADDSB(uint32_t instr, EE_JIT::IRInstruction& i); // 0x30

	void EmitPSUBB(uint32_t instr, EE_JIT::IRInstruction& i); // 0x09

	// MMI2
	void EmitPMFHI(uint32_t instr, EE_JIT::IRInstruction& i); // 0x08
	void EmitPMFLO(uint32_t instr, EE_JIT::IRInstruction& i); // 0x09
	void EmitPCPYLD(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0e
	void EmitPAND(uint32_t instr, EE_JIT::IRInstruction& i); // 0x12

	// MMI3
	void EmitPCPYUD(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0E
	void EmitPOR(uint32_t instr, EE_JIT::IRInstruction& i); // 0x12
	void EmitPNOR(uint32_t instr, EE_JIT::IRInstruction& i); // 0x13
	void EmitPCPYH(uint32_t instr, EE_JIT::IRInstruction& i); // 0x1B

	void EmitPADDUW(uint32_t instr, EE_JIT::IRInstruction& i); // 0x10

	// COP0
	void EmitMFC0(uint32_t instr, EE_JIT::IRInstruction& i); // 0x00
	void EmitMTC0(uint32_t instr, EE_JIT::IRInstruction& i); // 0x04
	void EmitERET(uint32_t instr, EE_JIT::IRInstruction& i); // 0x18

	// COP1
	void EmitMFC1(uint32_t instr, EE_JIT::IRInstruction& i); // 0x00
	void EmitMTC1(uint32_t instr, EE_JIT::IRInstruction& i); // 0x04

	// COP1 BC
	void EmitBC1F(uint32_t instr, EE_JIT::IRInstruction& i); // 0x00
	void EmitBC1T(uint32_t instr, EE_JIT::IRInstruction& i); // 0x01
	
	// COP1.S
	void EmitADDS(uint32_t instr, EE_JIT::IRInstruction& i); // 0x00
	void EmitSUBS(uint32_t instr, EE_JIT::IRInstruction& i); // 0x01
	void EmitMULS(uint32_t instr, EE_JIT::IRInstruction& i); // 0x02
	void EmitDIVS(uint32_t instr, EE_JIT::IRInstruction& i); // 0x03
	void EmitMOVS(uint32_t instr, EE_JIT::IRInstruction& i); // 0x06
	void EmitNEGS(uint32_t instr, EE_JIT::IRInstruction& i); // 0x07
	void EmitADDAS(uint32_t instr, EE_JIT::IRInstruction& i); // 0x18
	void EmitMADDS(uint32_t instr, EE_JIT::IRInstruction& i); // 0x1c
	void EmitCVTW(uint32_t instr, EE_JIT::IRInstruction& i); // 0x24
	void EmitCEQS(uint32_t instr, EE_JIT::IRInstruction& i); // 0x32
	void EmitCLES(uint32_t instr, EE_JIT::IRInstruction& i); // 0x36

	// COP1.W
	void EmitCVTS(uint32_t instr, EE_JIT::IRInstruction& i); // 0x30

	// COP2
	void EmitQMFC2(uint32_t instr, EE_JIT::IRInstruction& i); // 0x01
	void EmitCFC2(uint32_t instr, EE_JIT::IRInstruction& i); // 0x02
	void EmitQMTC2(uint32_t instr, EE_JIT::IRInstruction& i); // 0x05
	void EmitCTC2(uint32_t instr, EE_JIT::IRInstruction& i); // 0x06

	// COP2 special1
	void EmitVMADDZ(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0a
	void EmitVMADDW(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0b
	void EmitVSUB(uint32_t instr, EE_JIT::IRInstruction& i); // 0x2c
	void EmitVIADD(uint32_t instr, EE_JIT::IRInstruction& i); // 0x30

	// COP2 special2
	void EmitVMADDAX(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0a
	void EmitVMADDAY(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0b
	void EmitVMADDAZ(uint32_t instr, EE_JIT::IRInstruction& i); // 0x0b
	void EmitVMULAX(uint32_t instr, EE_JIT::IRInstruction& i); // 0x18
	void EmitVMULAW(uint32_t instr, EE_JIT::IRInstruction& i); // 0x1B
	void EmitVCLIPW(uint32_t instr, EE_JIT::IRInstruction& i); // 0x1F
	void EmitVSQI(uint32_t instr, EE_JIT::IRInstruction& i); // 0x35
	void EmitVISWR(uint32_t instr, EE_JIT::IRInstruction& i); // 0x3f

	// BC0
	void EmitBC0F(uint32_t instr, EE_JIT::IRInstruction& i); // 0x00
	void EmitBC0T(uint32_t instr, EE_JIT::IRInstruction& i); // 0x01

	void EmitIncPC(EE_JIT::IRInstruction& i);
public:
	using EntryFunc = void(*)();

	void EmitIR(uint32_t instr);

	void EmitPrequel(uint32_t guest_start);
	EntryFunc EmitDone(size_t cycles_taken);
	Block* GetExistingBlock(uint32_t start);

	bool DoesBlockExist(uint32_t addr);
	void CheckCacheFull();

	void MarkDirty(uint32_t address, uint32_t size);
};

}

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
int Clock();
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
EE_JIT::JIT::EntryFunc EmitDone(uint64_t cycles_taken);
EE_JIT::JIT::EntryFunc GetExistingBlockFunc(uint32_t addr);
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