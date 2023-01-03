#include "EmotionEngine.h"
#include <emu/memory/Bus.h>

namespace EE_JIT
{

enum IRInstrs
{
	MOV = 0,
	NOP = 0,
	SLTI = 0,
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
	// Can be used for guest and COP0 registers
	void SetReg(uint32_t reg) {value.register_num = reg;}
	void SetImm(uint16_t imm) {value.imm = (int32_t)(int16_t)imm;}
};

struct IRInstruction
{
	uint8_t instr;
	// Arguments are right -> left
	std::vector<IRValue> args;
};

void JIT::EmitSLTI(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);
	source.SetReg(rs);
	dest.SetReg(rt);
	imm.SetImm(instr & 0xffff);

	i.instr = IRInstrs::SLTI;
	i.args.push_back(imm);
	i.args.push_back(source);
	i.args.push_back(dest);

	cur_block->ir.push_back(i);
}

void JIT::EmitCOP0(uint32_t instr, EE_JIT::IRInstruction &i)
{
	uint8_t op = (instr >> 21) & 0x1F;

	switch (op)
	{
	case 0x00:
	{
		EmitMFC0(instr, i);
		break;
	}
	default:
		printf("[emu/CPU]: Cannot emit unknown cop0 instruction 0x%02x\n", op);
		exit(1);
	}
}

void JIT::EmitMFC0(uint32_t instr, EE_JIT::IRInstruction& i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Cop0Reg);

	dest.SetReg(rd);
	src.SetReg(rt);

	i.instr = IRInstrs::MOV;
	i.args.push_back(src);
	i.args.push_back(dest);

	cur_block->ir.push_back(i);
}

void JIT::EmitIR(uint32_t instr)
{
	if (!cur_block)
	{
		cur_block = new Block;
	}

	EE_JIT::IRInstruction current_instruction;

	if (instr == 0)
	{
		current_instruction.instr = NOP;
		cur_block->ir.push_back(current_instruction);
		return;
	}

	uint8_t opcode = (instr >> 26) & 0x3F;

	switch (opcode)
	{
	case 0x0A:
		EmitSLTI(instr, current_instruction);
		break;
	case 0x10:
		EmitCOP0(instr, current_instruction);
		break;
	default:
		printf("[emu/CPU]: Cannot emit unknown instruction 0x%02x\n", opcode);
		exit(1);
	}
}

}

EE_JIT::JIT jit;

namespace EmotionEngine
{
ProcessorState state;

void Reset()
{
	memset(&state, 0, sizeof(state));

	state.pc = 0xBFC00000;
	state.next_pc = 0xBFC00004;
}

void Clock()
{
	uint32_t instr = Bus::Read32(state.pc);
	state.pc = state.next_pc;
	state.next_pc += 4;

	jit.EmitIR(instr);
}

}
