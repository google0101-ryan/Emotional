#include "EmotionEngine.h"
#include <emu/memory/Bus.h>
#include <emu/cpu/ee/x64/emit.h>

namespace EE_JIT
{

void JIT::EmitBNE(uint32_t instr, EE_JIT::IRInstruction &i)
{
	printf("bne\n");

	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);

	source.SetReg(rs);
	dest.SetReg(rt);
	imm.SetImm(instr & 0xffff);

	i = IRInstruction::Build({dest, source, imm}, IRInstrs::BranchConditional);
	i.b_type = IRInstruction::BranchType::NE;
	cur_block->ir.push_back(i);
}

void JIT::EmitSLTI(uint32_t instr, EE_JIT::IRInstruction &i)
{
	printf("slti\n");

	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);
	source.SetReg(rs);
	dest.SetReg(rt);
	imm.SetImm(instr & 0xffff);

	i = IRInstruction::Build({dest, source, imm}, IRInstrs::SLTI);

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
	printf("mfc0\n");

	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Cop0Reg);

	dest.SetReg(rd);
	src.SetReg(rt);
	
	i = IRInstruction::Build({dest, src}, IRInstrs::MOV);

	cur_block->ir.push_back(i);
}

void JIT::EmitIR(uint32_t instr)
{
	EE_JIT::IRInstruction current_instruction;

	if (instr == 0)
	{
		printf("nop\n");
		current_instruction.instr = NOP;
		cur_block->ir.push_back(current_instruction);
		return;
	}

	uint8_t opcode = (instr >> 26) & 0x3F;

	switch (opcode)
	{
	case 0x05:
		EmitBNE(instr, current_instruction);
		break;
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

void JIT::EmitPrequel()
{
	if (!cur_block)
	{
		cur_block = new Block;
	}

	auto i = IRInstruction::Build({}, IRInstrs::SaveHostRegs);
	cur_block->ir.push_back(i);
}

void JIT::EmitDone()
{
	auto i = IRInstruction::Build({}, IRInstrs::RestoreHostRegs);
	cur_block->ir.push_back(i);

	blockCache.push_back(cur_block);
	EE_JIT::emit->TranslateBlock(cur_block);
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

	EE_JIT::emit = new EE_JIT::Emitter();
}

bool IsBranch(uint32_t instr)
{
	uint8_t opcode = (instr >> 26) & 0x3F;

	switch (opcode)
	{
	case 0x05:
		return true;
	default:
		return false;
	}
}

void Clock()
{
	jit.EmitPrequel();

	bool isBranch = false;
	bool isBranchDelayed = false;

	do
	{
		uint32_t instr = Bus::Read32(state.pc);
		state.pc = state.next_pc;
		state.next_pc += 4;

		jit.EmitIR(instr);

		isBranch = isBranchDelayed;
		isBranchDelayed = IsBranch(instr);
	} while (!isBranch);

	jit.EmitDone();
	exit(1);
}

void Dump()
{
	EE_JIT::emit->Dump();
}

ProcessorState* GetState()
{
	return &state;
}
}
