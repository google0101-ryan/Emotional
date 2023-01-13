// (c) Copyright 2022 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include "EmotionEngine.h"
#include <emu/memory/Bus.h>
#include <emu/cpu/ee/x64/emit.h>

namespace EE_JIT
{

void JIT::EmitJAL(uint32_t instr, EE_JIT::IRInstruction &i)
{
	printf("jal 0x%08x\n", (instr & 0x3ffffff) << 2);

	uint32_t target_address = (instr & 0x3FFFFFF) << 2;

	IRValue imm = IRValue(IRValue::Imm);
	imm.SetImm32Unsigned(target_address);

	i = IRInstruction::Build({imm}, IRInstrs::JumpImm);
	i.should_link = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitBEQ(uint32_t instr, EE_JIT::IRInstruction &i)
{
	printf("beq\n");

	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);


	source.SetReg(rs);
	dest.SetReg(rt);
	imm.SetImm(instr & 0xffff);

	i = IRInstruction::Build({dest, source, imm}, IRInstrs::BranchConditional);
	i.b_type = IRInstruction::BranchType::EQ;
	cur_block->ir.push_back(i);
}

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

void JIT::EmitADDIU(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	printf("addiu %s, %s, %d\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), (int32_t)(int16_t)(instr & 0xffff));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);

	source.SetReg(rs);
	dest.SetReg(rt);
	imm.SetImm(instr & 0xffff);

	i = IRInstruction::Build({dest, source, imm}, IRInstrs::Add);
	i.size = IRInstruction::Size32;
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

void JIT::EmitANDI(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	printf("andi %s, %s, 0x%04x\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), instr & 0xffff);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);
	source.SetReg(rs);
	dest.SetReg(rt);
	imm.SetImmUnsigned(instr & 0xffff);

	i = IRInstruction::Build({dest, source, imm}, IRInstrs::AND);

	cur_block->ir.push_back(i);
}

void JIT::EmitORI(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	printf("ori %s, %s, 0x%04x\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), instr & 0xffff);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);
	source.SetReg(rs);
	dest.SetReg(rt);
	imm.SetImmUnsigned(instr & 0xffff);

	i = IRInstruction::Build({dest, source, imm}, IRInstrs::OR);

	cur_block->ir.push_back(i);
}

void JIT::EmitLUI(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;

	printf("lui %s, 0x%04x\n", EmotionEngine::Reg(rt), instr & 0xffff);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);
	dest.SetReg(rt);
	imm.SetImm32((instr & 0xffff) << 16);

	i = IRInstruction::Build({dest, imm}, IRInstrs::MOV);

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
	case 0x04:
	{
		EmitMTC0(instr, i);
		break;
	}
	case 0x10:
		printf("tlbwi\n");
		break;
	default:
		printf("[emu/CPU]: Cannot emit unknown cop0 instruction 0x%02x\n", op);
		exit(1);
	}
}

void JIT::EmitSW(uint32_t instr, EE_JIT::IRInstruction &i)
{
	printf("sw\n");

	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::MemoryStore);
	i.access_size = IRInstruction::U32;

	cur_block->ir.push_back(i);
}

void JIT::EmitSD(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);
	
	printf("sd %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::MemoryStore);
	i.access_size = IRInstruction::U64;

	cur_block->ir.push_back(i);
}

void JIT::EmitSLL(uint32_t instr, EE_JIT::IRInstruction& i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int sa = (instr >> 6) & 0x1F;

	printf("sll %s, %s, %d\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), sa);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);
	IRValue shift = IRValue(IRValue::Imm);
	
	dest.SetReg(rd);
	src.SetReg(rt);
	shift.SetImm32Unsigned(sa);

	i = IRInstruction::Build({dest, src, shift}, IRInstrs::Shift);
	i.is_logical = true;
	i.direction = IRInstruction::Left;

	cur_block->ir.push_back(i);
}

void JIT::EmitJR(uint32_t instr, EE_JIT::IRInstruction &i)
{
	printf("jr\n");

	int rs = (instr >> 21) & 0x1F;

	IRValue src = IRValue(IRValue::Reg);
	src.SetReg(rs);

	i = IRInstruction::Build({src}, IRInstrs::JumpAlways);

	cur_block->ir.push_back(i);
}

void JIT::EmitJALR(uint32_t instr, EE_JIT::IRInstruction &i)
{
	printf("jalr\n");

	int rs = (instr >> 21) & 0x1F;
	int rd = (instr >> 11) & 0x1F;

	IRValue src = IRValue(IRValue::Reg);
	IRValue link = IRValue(IRValue::Reg);

	src.SetReg(rs);
	link.SetReg(rd);

	i = IRInstruction::Build({src, link}, IRInstrs::JumpAlways);
	i.should_link = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitOR(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	printf("or %s, %s, %s\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), EmotionEngine::Reg(rd));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue source2 = IRValue(IRValue::Reg);
	source.SetReg(rs);
	dest.SetReg(rt);
	source2.SetReg(rd);

	i = IRInstruction::Build({dest, source, source2}, IRInstrs::OR);

	cur_block->ir.push_back(i);
}

void JIT::EmitDADDU(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	printf("daddu %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);
	IRValue dest = IRValue(IRValue::Reg);

	src1.SetReg(rs);
	src2.SetReg(rt);
	dest.SetReg(rd);

	i = IRInstruction::Build({dest, src1, src2}, IRInstrs::Add);
	i.size = IRInstruction::Size64;

	cur_block->ir.push_back(i);
}

void JIT::EmitMFC0(uint32_t instr, EE_JIT::IRInstruction& i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;

	printf("mfc0 %s, r%d\n", EmotionEngine::Reg(rt), rd);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Cop0Reg);

	dest.SetReg(rt);
	src.SetReg(rd);
	
	i = IRInstruction::Build({dest, src}, IRInstrs::MOV);

	cur_block->ir.push_back(i);
}

void JIT::EmitMTC0(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;

	printf("mtc0 %s, r%d\n", EmotionEngine::Reg(rt), rd);

	IRValue dest = IRValue(IRValue::Cop0Reg);
	IRValue src = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src.SetReg(rt);
	
	i = IRInstruction::Build({dest, src}, IRInstrs::MOV);

	cur_block->ir.push_back(i);
}

void JIT::EmitIncPC(EE_JIT::IRInstruction &i)
{
	i = IRInstruction::Build({}, IRInstrs::IncPC);

	cur_block->ir.push_back(i);
}

void JIT::EmitIR(uint32_t instr)
{
	EE_JIT::IRInstruction current_instruction;

	EmitIncPC(current_instruction);

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
	case 0x00:
	{
		opcode = instr & 0x3F;
		switch (opcode)
		{		
		case 0x00:
			EmitSLL(instr, current_instruction);
			break;	
		case 0x08:
			EmitJR(instr, current_instruction);
			break;
		case 0x09:
			EmitJALR(instr, current_instruction);
			break;
		case 0x0F:
			printf("sync\n");
			break;
		case 0x25:
			EmitOR(instr, current_instruction);
			break;
		case 0x2D:
			EmitDADDU(instr, current_instruction);
			break;
		default:
			printf("[emu/CPU]: Cannot emit unknown special instruction 0x%02x\n", opcode);
			cur_block->ir.clear();
			delete cur_block;
			exit(1);
		}
		break;
	}
	case 0x03:
		EmitJAL(instr, current_instruction);
		break;
	case 0x04:
		EmitBEQ(instr, current_instruction);
		break;
	case 0x05:
		EmitBNE(instr, current_instruction);
		break;
	case 0x09:
		EmitADDIU(instr, current_instruction);
		break;
	case 0x0A:
		EmitSLTI(instr, current_instruction);
		break;
	case 0x0C:
		EmitANDI(instr, current_instruction);
		break;
	case 0x0D:
		EmitORI(instr, current_instruction);
		break;
	case 0x0F:
		EmitLUI(instr, current_instruction);
		break;
	case 0x10:
		EmitCOP0(instr, current_instruction);
		break;
	case 0x2b:
		EmitSW(instr, current_instruction);
		break;
	case 0x3f:
		EmitSD(instr, current_instruction);
		break;
	default:
		printf("[emu/CPU]: Cannot emit unknown instruction 0x%02x\n", opcode);
		delete cur_block;
		exit(1);
	}
}

void JIT::EmitPrequel(uint32_t guest_start)
{
	printf("---------------------\n");
	cur_block = new Block;
	cur_block->guest_start = guest_start;
	cur_block->entry = (uint8_t*)emit->GetFreeBase();

	auto i = IRInstruction::Build({}, IRInstrs::SaveHostRegs);
	cur_block->ir.push_back(i);
}

JIT::EntryFunc JIT::EmitDone()
{
	auto i = IRInstruction::Build({}, IRInstrs::RestoreHostRegs);
	cur_block->ir.push_back(i);

	blockCache.push_back(cur_block);
	EE_JIT::emit->TranslateBlock(cur_block);
	return (EntryFunc)cur_block->entry;
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
	state.cop0_regs[15] = 0x2E20;

	EE_JIT::emit = new EE_JIT::Emitter();
}

bool IsBranch(uint32_t instr)
{
	uint8_t opcode = (instr >> 26) & 0x3F;

	switch (opcode)
	{
	case 0x00:
	{
		opcode = instr & 0x3F;
		switch (opcode)
		{			
		case 0x08:	
		case 0x09:
			return true;
		default:
			return false;
		}
		break;
	}
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
		return true;
	default:
		return false;
	}
}

int Clock()
{
	jit.EmitPrequel(state.pc);

	bool isBranch = false;
	bool isBranchDelayed = false;

	uint32_t pc = state.pc;
	uint32_t next_pc = state.next_pc;

	int cycles = 0;

	do
	{
		uint32_t instr = Bus::Read32(pc);
		pc = next_pc;
		next_pc += 4;

		jit.EmitIR(instr);

		isBranch = isBranchDelayed;
		isBranchDelayed = IsBranch(instr);

		cycles += 2;
	} while (!isBranch);

	auto func = jit.EmitDone();
	func();

	return cycles;
}

void Dump()
{
	EE_JIT::emit->Dump();

	for (int i = 0; i < 32; i++)
		printf("%s\t->\t0x%lx%016lx\n", Reg(i), state.regs[i].u64[1], state.regs[i].u64[0]);
	printf("pc\t->\t0x%08x\n", state.pc);
}

ProcessorState* GetState()
{
	return &state;
}
}
