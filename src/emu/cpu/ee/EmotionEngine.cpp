// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include "EmotionEngine.h"
#include <emu/memory/Bus.h>
#include <emu/cpu/ee/x64/emit.h>


extern float convert(uint32_t);

namespace EmotionEngine
{
	ProcessorState state;
	bool can_dump = false;
	bool can_disassemble = false;
}

namespace EE_JIT
{

void JIT::EmitJ(uint32_t instr, EE_JIT::IRInstruction &i)
{
	if (EmotionEngine::can_disassemble)
		printf("j 0x%08x\n", (instr & 0x3ffffff) << 2);

	uint32_t target_address = (instr & 0x3FFFFFF) << 2;

	IRValue imm = IRValue(IRValue::Imm);
	imm.SetImm32Unsigned(target_address);

	i = IRInstruction::Build({imm}, IRInstrs::JumpImm);
	i.should_link = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitJAL(uint32_t instr, EE_JIT::IRInstruction &i)
{
	if (EmotionEngine::can_disassemble)
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
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	int32_t i_ = (int32_t)(int16_t)(instr & 0xffff) << 2;

	if (EmotionEngine::can_disassemble)
		printf("beq %s, %s, 0x%08x (%x)\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_, i_);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);

	source.SetReg(rs);
	dest.SetReg(rt);
	imm.SetImm32((int32_t)(int16_t)(instr & 0xffff) << 2);

	i = IRInstruction::Build({dest, source, imm}, IRInstrs::BranchConditional);
	i.b_type = IRInstruction::BranchType::EQ;
	cur_block->ir.push_back(i);
}

void JIT::EmitBNE(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;
	uint32_t i_ = (int32_t)(int16_t)(instr & 0xffff) << 2;

	if (EmotionEngine::can_disassemble)
		printf("bne %s, %s, 0x%08x\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);


	source.SetReg(rs);
	dest.SetReg(rt);
	imm.SetImm32((int32_t)(int16_t)(instr & 0xffff) << 2);

	i = IRInstruction::Build({dest, source, imm}, IRInstrs::BranchConditional);
	i.b_type = IRInstruction::BranchType::NE;
	cur_block->ir.push_back(i);
}

void JIT::EmitBLEZ(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rs = (instr >> 21) & 0x1F;

	int32_t i_ = (int32_t)(int16_t)(instr & 0xffff) << 2;

	if (EmotionEngine::can_disassemble)
		printf("blez %s, 0x%08x (%x)\n", EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_, i_);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);

	dest.SetReg(rs);
	imm.SetImm32((int32_t)(int16_t)(instr & 0xffff) << 2);

	i = IRInstruction::Build({dest, imm}, IRInstrs::BranchRegImm);
	i.b_type = IRInstruction::BranchType::LE;
	i.is_likely = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitBGTZ(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rs = (instr >> 21) & 0x1F;

	int32_t i_ = (int32_t)(int16_t)(instr & 0xffff) << 2;

	if (EmotionEngine::can_disassemble)
		printf("bgtz %s, 0x%08x (%x)\n", EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_, i_);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);

	dest.SetReg(rs);
	imm.SetImm32((int32_t)(int16_t)(instr & 0xffff) << 2);

	i = IRInstruction::Build({dest, imm}, IRInstrs::BranchRegImm);
	i.b_type = IRInstruction::BranchType::GT;
	i.is_likely = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitADDI(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("addi %s, %s, %d\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), (int32_t)(int16_t)(instr & 0xffff));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);

	source.SetReg(rs);
	dest.SetReg(rt);
	imm.SetImm((int32_t)(int16_t)(instr & 0xffff));

	i = IRInstruction::Build({dest, source, imm}, IRInstrs::Add);
	i.size = IRInstruction::Size32;
	cur_block->ir.push_back(i);
}

void JIT::EmitADDIU(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("addiu %s, %s, %d\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), (int32_t)(int16_t)(instr & 0xffff));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);

	source.SetReg(rs);
	dest.SetReg(rt);
	imm.SetImm((int32_t)(int16_t)(instr & 0xffff));

	i = IRInstruction::Build({dest, source, imm}, IRInstrs::Add);
	i.size = IRInstruction::Size32;
	cur_block->ir.push_back(i);
}

void JIT::EmitSLTI(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("slti %s, %s, 0x%02x\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), (int32_t)(int16_t)(instr & 0xffff));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);
	source.SetReg(rs);
	dest.SetReg(rt);
	imm.SetImm((int32_t)(int16_t)(instr & 0xffff));

	i = IRInstruction::Build({dest, source, imm}, IRInstrs::SLTI);
	i.is_unsigned = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitSLTIU(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;
	uint16_t i_ = instr & 0xffff;

	if (EmotionEngine::can_disassemble)
		printf("sltiu %s, %s, 0x%04x\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), i_);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);

	source.SetReg(rs);
	dest.SetReg(rt);
	imm.SetImm((int32_t)(int16_t)(instr & 0xffff));

	i = IRInstruction::Build({dest, source, imm}, IRInstrs::SLTI);
	i.is_unsigned = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitANDI(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
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

	if (EmotionEngine::can_disassemble)
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

void JIT::EmitXORI(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;
	
	if (EmotionEngine::can_disassemble)
		printf("xori %s, %s, 0x%04x\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), instr & 0xffff);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);
	source.SetReg(rs);
	dest.SetReg(rt);
	imm.SetImmUnsigned(instr & 0xffff);

	i = IRInstruction::Build({dest, source, imm}, IRInstrs::XOR);

	cur_block->ir.push_back(i);
}

void JIT::EmitLUI(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("lui %s, 0x%04x\n", EmotionEngine::Reg(rt), instr & 0xffff);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);
	dest.SetReg(rt);
	imm.SetImm32(((int32_t)(((int16_t)(instr & 0xffff))) << 16));

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
	case 0x08:
	{
		uint8_t opcode = (instr >> 16) & 0x1F;

		switch (opcode)
		{
		case 0x00:
			EmitBC0F(instr, i);
			break;
		case 0x01:
			EmitBC0T(instr, i);
			break;
		default:
			printf("[emu/CPU]: Cannot emit unknown cop0 BC0 instruction 0x%02x\n", opcode);
			exit(1);
		}

		break;
	}
	case 0x10:
	{
		uint8_t opcode = (instr & 0x3F);

		switch (opcode)
		{
		case 0x01:
		case 0x02:
			break;
		case 0x18:
			EmitERET(instr, i);
			break;
		case 0x38:
			i = IRInstruction::Build({}, IRInstrs::EI);
			cur_block->ir.push_back(i);
			break;
		case 0x39:
			i = IRInstruction::Build({}, IRInstrs::DI);
			cur_block->ir.push_back(i);
			break;
		default:
			printf("[emu/CPU]: Cannot emit unknown cop0 TLB instruction 0x%02x\n", opcode);
			exit(1);
		}

		break;
	}
	default:
		printf("[emu/CPU]: Cannot emit unknown cop0 instruction 0x%02x\n", op);
		exit(1);
	}
}

void JIT::EmitBEQL(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;
	int32_t i_ = (int32_t)(int16_t)(instr & 0xffff) << 2;

	if (EmotionEngine::can_disassemble)
		printf("beql %s, %s, 0x%08x\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);


	source.SetReg(rs);
	dest.SetReg(rt);
	imm.SetImm32((int32_t)(int16_t)(instr & 0xffff) << 2);

	i = IRInstruction::Build({dest, source, imm}, IRInstrs::BranchConditional);
	i.b_type = IRInstruction::BranchType::EQ;
	i.is_likely = true;
	cur_block->ir.push_back(i);
}

void JIT::EmitBNEL(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;
	uint32_t i_ = (int32_t)(int16_t)(instr & 0xffff) << 2;

	if (EmotionEngine::can_disassemble)
		printf("bnel %s, %s, 0x%08x\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);


	source.SetReg(rs);
	dest.SetReg(rt);
	imm.SetImm32((int32_t)(int16_t)(instr & 0xffff) << 2);

	i = IRInstruction::Build({dest, source, imm}, IRInstrs::BranchConditional);
	i.b_type = IRInstruction::BranchType::NE;
	i.is_likely = true;
	cur_block->ir.push_back(i);
}

void JIT::EmitDADDIU(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("daddiu %s, %s, %d\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), (int32_t)(int16_t)(instr & 0xffff));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);

	source.SetReg(rs);
	dest.SetReg(rt);
	imm.SetImm((int32_t)(int16_t)(instr & 0xffff));

	i = IRInstruction::Build({dest, source, imm}, IRInstrs::Add);
	i.size = IRInstruction::Size64;
	cur_block->ir.push_back(i);
}

void JIT::EmitLDL(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
		printf("ldl %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::LDL);
	i.opcode = instr;

	cur_block->ir.push_back(i);
}

void JIT::EmitLDR(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
		printf("ldr %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::LDR);
	i.opcode = instr;

	cur_block->ir.push_back(i);
}

void JIT::EmitLQ(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
		printf("lq %s, %d(%s) (0x%08x)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base), EmotionEngine::state.regs[_base].u32[0] + _offset);

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::MemoryLoad);
	i.access_size = IRInstruction::U128;
	i.is_unsigned = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitSQ(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
		printf("sq %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::MemoryStore);
	i.access_size = IRInstruction::U128;

	cur_block->ir.push_back(i);
}

void JIT::EmitLB(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
		printf("lb %s, %d(%s) (0x%08x)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base), EmotionEngine::state.regs[_base].u32[0] + _offset);

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::MemoryLoad);
	i.access_size = IRInstruction::U8;
	i.is_unsigned = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitLH(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
		printf("lh %s, %d(%s) (0x%08x)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base), EmotionEngine::state.regs[_base].u32[0] + _offset);

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::MemoryLoad);
	i.access_size = IRInstruction::U16;
	i.is_unsigned = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitLW(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
		printf("lw %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::MemoryLoad);
	i.access_size = IRInstruction::U32;
	i.is_unsigned = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitLBU(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
		printf("lbu %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::MemoryLoad);
	i.access_size = IRInstruction::U8;
	i.is_unsigned = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitLHU(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
		printf("lhu %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::MemoryLoad);
	i.access_size = IRInstruction::U16;
	i.is_unsigned = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitLWU(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
		printf("lwu %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::MemoryLoad);
	i.access_size = IRInstruction::U32;
	i.is_unsigned = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitSB(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
		printf("sb %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::MemoryStore);
	i.access_size = IRInstruction::U8;

	cur_block->ir.push_back(i);
}

void JIT::EmitSH(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
		printf("sh %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::MemoryStore);
	i.access_size = IRInstruction::U16;

	cur_block->ir.push_back(i);
}

void JIT::EmitSW(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
		printf("sw %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::MemoryStore);
	i.access_size = IRInstruction::U32;
	i.is_unsigned = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitSDL(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
		printf("sdl %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::SDL);
	i.opcode = instr;

	cur_block->ir.push_back(i);
}

void JIT::EmitSDR(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
		printf("sdr %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::SDR);
	i.opcode = instr;

	cur_block->ir.push_back(i);
}

void JIT::EmitLWC1(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
		printf("swc1 %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Float);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::MemoryLoad);
	i.access_size = IRInstruction::U32;

	cur_block->ir.push_back(i);
}

void JIT::EmitLQC2(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);
	i = IRInstruction::Build({inst}, IRInstrs::LQC2);

	cur_block->ir.push_back(i);
}

void JIT::EmitLD(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
		printf("ld %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::MemoryLoad);
	i.access_size = IRInstruction::U64;
	i.is_unsigned = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitSWC1(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
		printf("swc1 %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Float);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::MemoryStore);
	i.access_size = IRInstruction::U32;

	cur_block->ir.push_back(i);
}

void JIT::EmitSQC2(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);
	i = IRInstruction::Build({inst}, IRInstrs::SQC2);

	cur_block->ir.push_back(i);
}

void JIT::EmitSD(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	if (EmotionEngine::can_disassemble)
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

	if (EmotionEngine::can_disassemble)
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

void JIT::EmitSRL(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int sa = (instr >> 6) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("srl %s, %s, %d\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), sa);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);
	IRValue shift = IRValue(IRValue::Imm);

	dest.SetReg(rd);
	src.SetReg(rt);
	shift.SetImm32Unsigned(sa);

	i = IRInstruction::Build({dest, src, shift}, IRInstrs::Shift);
	i.is_logical = true;
	i.direction = IRInstruction::Right;

	cur_block->ir.push_back(i);
}

void JIT::EmitSRA(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int sa = (instr >> 6) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("sra %s, %s, %d\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), sa);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);
	IRValue shift = IRValue(IRValue::Imm);

	dest.SetReg(rd);
	src.SetReg(rt);
	shift.SetImm32Unsigned(sa);

	i = IRInstruction::Build({dest, src, shift}, IRInstrs::Shift);
	i.is_logical = false;
	i.direction = IRInstruction::Right;

	cur_block->ir.push_back(i);
}

void JIT::EmitSLLV(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("sllv %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);
	IRValue shamt = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src.SetReg(rt);
	shamt.SetReg(rs);

	i = IRInstruction::Build({dest, src, shamt}, IRInstrs::Shift);
	i.direction = IRInstruction::Direction::Left;
	i.is_logical = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitSRLV(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("srlv %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);
	IRValue shamt = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src.SetReg(rt);
	shamt.SetReg(rs);

	i = IRInstruction::Build({dest, src, shamt}, IRInstrs::Shift);
	i.direction = IRInstruction::Direction::Right;
	i.is_logical = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitSRAV(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("srav %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);
	IRValue shamt = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src.SetReg(rt);
	shamt.SetReg(rs);

	i = IRInstruction::Build({dest, src, shamt}, IRInstrs::Shift);
	i.direction = IRInstruction::Direction::Right;
	i.is_logical = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitJR(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("jr %s\n", EmotionEngine::Reg(rs));

	IRValue src = IRValue(IRValue::Reg);
	src.SetReg(rs);

	i = IRInstruction::Build({src}, IRInstrs::JumpAlways);

	cur_block->ir.push_back(i);
}

void JIT::EmitJALR(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rs = (instr >> 21) & 0x1F;
	int rd = (instr >> 11) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("jalr %s,%s\n", EmotionEngine::Reg(rs), EmotionEngine::Reg(rd));

	IRValue src = IRValue(IRValue::Reg);
	IRValue link = IRValue(IRValue::Reg);

	src.SetReg(rs);
	link.SetReg(rd);

	i = IRInstruction::Build({src, link}, IRInstrs::JumpAlways);
	i.should_link = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitMOVZ(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("movz %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src1.SetReg(rt);
	src2.SetReg(rs);

	i = IRInstruction::Build({dest, src1, src2}, IRInstrs::MovCond);
	i.mov_cond = IRInstruction::MovCond::Z;

	cur_block->ir.push_back(i);
}

void JIT::EmitMOVN(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("movn %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src1.SetReg(rt);
	src2.SetReg(rs);

	i = IRInstruction::Build({dest, src1, src2}, IRInstrs::MovCond);
	i.mov_cond = IRInstruction::MovCond::N;

	cur_block->ir.push_back(i);
}

void JIT::EmitSyscall(uint32_t instr, EE_JIT::IRInstruction &i)
{
	if (EmotionEngine::can_disassemble)
		printf("syscall\n");

	i = IRInstruction::Build({}, IRInstrs::SYSCALL);
	cur_block->ir.push_back(i);
}

void JIT::EmitBreak(uint32_t instr, EE_JIT::IRInstruction &i)
{
	if (EmotionEngine::can_disassemble)
		printf("break\n");

	i = IRInstruction::Build({}, IRInstrs::UhOh);
	cur_block->ir.push_back(i);
}

void JIT::EmitMFHI(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("mfhi %s\n", EmotionEngine::Reg(rd));

	IRValue reg = IRValue(IRValue::Reg);
	reg.SetReg(rd);

	i = IRInstruction::Build({reg}, IRInstrs::MoveFromHi);
	cur_block->ir.push_back(i);
}

void JIT::EmitMTHI(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("mthi %s\n", EmotionEngine::Reg(rd));

	IRValue reg = IRValue(IRValue::Reg);
	reg.SetReg(rd);

	i = IRInstruction::Build({reg}, IRInstrs::MTHI);
	cur_block->ir.push_back(i);
}

void JIT::EmitMFLO(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("mflo %s\n", EmotionEngine::Reg(rd));

	IRValue reg = IRValue(IRValue::Reg);
	reg.SetReg(rd);

	i = IRInstruction::Build({reg}, IRInstrs::MoveFromLo);
	i.is_mmi_divmul = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitMTLO(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("mtlo %s\n", EmotionEngine::Reg(rd));

	IRValue reg = IRValue(IRValue::Reg);
	reg.SetReg(rd);

	i = IRInstruction::Build({reg}, IRInstrs::MTLO);
	cur_block->ir.push_back(i);
}

void JIT::EmitDSLLV(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("dsllv %s, %s, %s (0x%08x)\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), instr);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);
	IRValue shamt = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src.SetReg(rt);
	shamt.SetReg(rs);

	i = IRInstruction::Build({dest, src, shamt}, IRInstrs::Shift64);
	i.direction = IRInstruction::Direction::Left;
	i.is_logical = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitDSRLV(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("dsrlv %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);
	IRValue shamt = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src.SetReg(rt);
	shamt.SetReg(rs);

	i = IRInstruction::Build({dest, src, shamt}, IRInstrs::Shift64);
	i.direction = IRInstruction::Direction::Right;
	i.is_logical = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitDSRAV(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("dsrav %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);
	IRValue shamt = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src.SetReg(rt);
	shamt.SetReg(rs);

	i = IRInstruction::Build({dest, src, shamt}, IRInstrs::Shift64);
	i.direction = IRInstruction::Direction::Right;
	i.is_logical = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitMULT(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("mult %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src1.SetReg(rt);
	src2.SetReg(rs);

	i = IRInstruction::Build({dest, src1, src2}, IRInstrs::Mult);
	i.is_unsigned = false;
	i.is_mmi_divmul = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitMULTU(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("multu %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src1.SetReg(rt);
	src2.SetReg(rs);

	i = IRInstruction::Build({dest, src1, src2}, IRInstrs::Mult);
	i.is_unsigned = true;
	i.is_mmi_divmul = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitDIV(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("div %s, %s\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);

	src1.SetReg(rt);
	src2.SetReg(rs);

	i = IRInstruction::Build({src1, src2}, IRInstrs::Div);
	i.is_unsigned = false;
	i.is_mmi_divmul = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitDIVU(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("divu %s, %s\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);

	src1.SetReg(rt);
	src2.SetReg(rs);

	i = IRInstruction::Build({src1, src2}, IRInstrs::Div);
	i.is_unsigned = true;
	i.is_mmi_divmul = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitADD(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("add %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue source2 = IRValue(IRValue::Reg);
	source.SetReg(rs);
	dest.SetReg(rd);
	source2.SetReg(rt);

	i = IRInstruction::Build({dest, source, source2}, IRInstrs::Add);

	cur_block->ir.push_back(i);
}

void JIT::EmitADDU(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("addu %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue source2 = IRValue(IRValue::Reg);
	source.SetReg(rs);
	dest.SetReg(rd);
	source2.SetReg(rt);

	i = IRInstruction::Build({dest, source, source2}, IRInstrs::Add);

	cur_block->ir.push_back(i);
}

void JIT::EmitSUB(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("sub %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue source2 = IRValue(IRValue::Reg);
	source.SetReg(rt);
	dest.SetReg(rd);
	source2.SetReg(rs);

	i = IRInstruction::Build({dest, source, source2}, IRInstrs::Sub);

	cur_block->ir.push_back(i);
}

void JIT::EmitSUBU(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("subu %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue source2 = IRValue(IRValue::Reg);
	source.SetReg(rt);
	dest.SetReg(rd);
	source2.SetReg(rs);

	i = IRInstruction::Build({dest, source, source2}, IRInstrs::Sub);

	cur_block->ir.push_back(i);
}

void JIT::EmitAND(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("and %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue source2 = IRValue(IRValue::Reg);
	source.SetReg(rs);
	dest.SetReg(rd);
	source2.SetReg(rt);

	i = IRInstruction::Build({dest, source, source2}, IRInstrs::AND);

	cur_block->ir.push_back(i);
}

void JIT::EmitOR(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("or %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue source2 = IRValue(IRValue::Reg);
	source.SetReg(rs);
	dest.SetReg(rd);
	source2.SetReg(rt);

	i = IRInstruction::Build({dest, source, source2}, IRInstrs::OR);

	cur_block->ir.push_back(i);
}

void JIT::EmitXOR(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("xor %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue source2 = IRValue(IRValue::Reg);
	source.SetReg(rs);
	dest.SetReg(rd);
	source2.SetReg(rt);

	i = IRInstruction::Build({dest, source, source2}, IRInstrs::XOR);

	cur_block->ir.push_back(i);
}

void JIT::EmitNOR(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("nor %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue source2 = IRValue(IRValue::Reg);
	source.SetReg(rs);
	dest.SetReg(rd);
	source2.SetReg(rt);

	i = IRInstruction::Build({dest, source, source2}, IRInstrs::NOR);

	cur_block->ir.push_back(i);
}

void JIT::EmitSLT(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("slt %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue source2 = IRValue(IRValue::Reg);
	source.SetReg(rs);
	dest.SetReg(rd);
	source2.SetReg(rt);

	i = IRInstruction::Build({dest, source, source2}, IRInstrs::SLTI);
	i.is_unsigned = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitSLTU(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("sltu %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue source2 = IRValue(IRValue::Reg);
	source.SetReg(rs);
	dest.SetReg(rd);
	source2.SetReg(rt);

	i = IRInstruction::Build({dest, source, source2}, IRInstrs::SLTI);
	i.is_unsigned = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitDADDU(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("daddu %s, %s, %s (0x%08lx, 0x%08lx)\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt), EmotionEngine::state.regs[rs].u64[0], EmotionEngine::state.regs[rt].u64[0]);

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

void JIT::EmitDSUBU(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("dsubu %s, %s, %s (0x%08lx, 0x%08lx)\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt), EmotionEngine::state.regs[rs].u64[0], EmotionEngine::state.regs[rt].u64[0]);

	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);
	IRValue dest = IRValue(IRValue::Reg);

	src1.SetReg(rs);
	src2.SetReg(rt);
	dest.SetReg(rd);

	i = IRInstruction::Build({dest, src1, src2}, IRInstrs::Sub);
	i.size = IRInstruction::Size64;

	cur_block->ir.push_back(i);
}

void JIT::EmitDSLL(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int sa = (instr >> 6) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("dsll %s, %s, %d\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), sa);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);
	IRValue shift = IRValue(IRValue::Imm);

	dest.SetReg(rd);
	src.SetReg(rt);
	shift.SetImm32Unsigned(sa);

	i = IRInstruction::Build({dest, src, shift}, IRInstrs::Shift64);
	i.is_logical = true;
	i.direction = IRInstruction::Left;

	cur_block->ir.push_back(i);
}

void JIT::EmitDSRL(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int sa = (instr >> 6) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("dsrl %s, %s, %d\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), sa);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);
	IRValue shift = IRValue(IRValue::Imm);

	dest.SetReg(rd);
	src.SetReg(rt);
	shift.SetImm32Unsigned(sa);

	i = IRInstruction::Build({dest, src, shift}, IRInstrs::Shift64);
	i.is_logical = true;
	i.direction = IRInstruction::Right;

	cur_block->ir.push_back(i);
}

void JIT::EmitDSLL32(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int sa = (instr >> 6) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("dsll32 %s, %s, %d\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), sa);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);
	IRValue shift = IRValue(IRValue::Imm);

	dest.SetReg(rd);
	src.SetReg(rt);
	shift.SetImm32Unsigned(sa+32);

	i = IRInstruction::Build({dest, src, shift}, IRInstrs::Shift64);
	i.is_logical = true;
	i.direction = IRInstruction::Left;

	cur_block->ir.push_back(i);
}

void JIT::EmitDSRL32(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int sa = (instr >> 6) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("dsrl32 %s, %s, %d\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), sa);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);
	IRValue shift = IRValue(IRValue::Imm);

	dest.SetReg(rd);
	src.SetReg(rt);
	shift.SetImm32Unsigned(sa+32);

	i = IRInstruction::Build({dest, src, shift}, IRInstrs::Shift64);
	i.is_logical = true;
	i.direction = IRInstruction::Right;

	cur_block->ir.push_back(i);
}

void JIT::EmitDSRA32(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int sa = (instr >> 6) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("dsra32 %s, %s, %d\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), sa);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);
	IRValue shift = IRValue(IRValue::Imm);

	dest.SetReg(rd);
	src.SetReg(rt);
	shift.SetImm32Unsigned(sa+32);

	i = IRInstruction::Build({dest, src, shift}, IRInstrs::Shift64);
	i.is_logical = false;
	i.direction = IRInstruction::Right;

	cur_block->ir.push_back(i);
}

void JIT::EmitBLTZ(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rs = (instr >> 21) & 0x1F;

	int32_t i_ = (int32_t)(int16_t)(instr & 0xffff) << 2;

	if (EmotionEngine::can_disassemble)
		printf("bltz %s, 0x%08x (%x)\n", EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_, i_);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);

	dest.SetReg(rs);
	imm.SetImm32((int32_t)(int16_t)(instr & 0xffff) << 2);

	i = IRInstruction::Build({dest, imm}, IRInstrs::BranchRegImm);
	i.b_type = IRInstruction::BranchType::LT;
	i.is_likely = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitBGEZ(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rs = (instr >> 21) & 0x1F;

	int32_t i_ = (int32_t)(int16_t)(instr & 0xffff) << 2;

	if (EmotionEngine::can_disassemble)
		printf("bgez %s, 0x%08x (%x)\n", EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_, i_);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);

	dest.SetReg(rs);
	imm.SetImm32((int32_t)(int16_t)(instr & 0xffff) << 2);

	i = IRInstruction::Build({dest, imm}, IRInstrs::BranchRegImm);
	i.b_type = IRInstruction::BranchType::GE;
	i.is_likely = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitBLTZL(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rs = (instr >> 21) & 0x1F;

	int32_t i_ = (int32_t)(int16_t)(instr & 0xffff) << 2;

	if (EmotionEngine::can_disassemble)
		printf("bltzl %s, 0x%08x (%x)\n", EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_, i_);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);

	dest.SetReg(rs);
	imm.SetImm32((int32_t)(int16_t)(instr & 0xffff) << 2);

	i = IRInstruction::Build({dest, imm}, IRInstrs::BranchRegImm);
	i.b_type = IRInstruction::BranchType::LT;
	i.is_likely = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitBGEZL(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rs = (instr >> 21) & 0x1F;

	int32_t i_ = (int32_t)(int16_t)(instr & 0xffff) << 2;

	if (EmotionEngine::can_disassemble)
		printf("bltzl %s, 0x%08x (%x)\n", EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_, i_);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);

	dest.SetReg(rs);
	imm.SetImm32((int32_t)(int16_t)(instr & 0xffff) << 2);

	i = IRInstruction::Build({dest, imm}, IRInstrs::BranchRegImm);
	i.b_type = IRInstruction::BranchType::GE;
	i.is_likely = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitBGEZAL(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rs = (instr >> 21) & 0x1F;

	int32_t i_ = (int32_t)(int16_t)(instr & 0xffff) << 2;

	if (EmotionEngine::can_disassemble)
		printf("bgezal %s, 0x%08x (%x)\n", EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_, i_);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);

	dest.SetReg(rs);
	imm.SetImm32((int32_t)(int16_t)(instr & 0xffff) << 2);

	i = IRInstruction::Build({dest, imm}, IRInstrs::BranchRegImm);
	i.b_type = IRInstruction::BranchType::GE;
	i.should_link = true;
	
	cur_block->ir.push_back(i);
}

void JIT::EmitMADD(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);
	i = IRInstruction::Build({inst}, IRInstrs::MADD);

	cur_block->ir.push_back(i);
}

void JIT::EmitPLZCW(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rs = (instr >> 21) & 0x1F;
	
	if (EmotionEngine::can_disassemble)
		printf("plzcw %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs));

	IRValue dst = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);

	dst.SetReg(rd);
	src.SetReg(rs);

	i = IRInstruction::Build({dst, src}, IRInstrs::PLZCW);
	
	cur_block->ir.push_back(i);
}

void JIT::EmitMFHI1(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("mfhi1 %s\n", EmotionEngine::Reg(rd));

	IRValue reg = IRValue(IRValue::Reg);
	reg.SetReg(rd);

	i = IRInstruction::Build({reg}, IRInstrs::MoveFromHi);
	i.is_mmi_divmul = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitMTHI1(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("mthi1 %s\n", EmotionEngine::Reg(rd));

	IRValue reg = IRValue(IRValue::Reg);
	reg.SetReg(rd);

	i = IRInstruction::Build({reg}, IRInstrs::MTHI);
	i.is_mmi_divmul = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitMFLO1(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("mflo1 %s\n", EmotionEngine::Reg(rd));

	IRValue reg = IRValue(IRValue::Reg);
	reg.SetReg(rd);

	i = IRInstruction::Build({reg}, IRInstrs::MoveFromLo);
	i.is_mmi_divmul = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitMTLO1(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("mthi1 %s\n", EmotionEngine::Reg(rd));

	IRValue reg = IRValue(IRValue::Reg);
	reg.SetReg(rd);

	i = IRInstruction::Build({reg}, IRInstrs::MTLO);
	i.is_mmi_divmul = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitMULT1(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("mult1 %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src1.SetReg(rt);
	src2.SetReg(rs);

	i = IRInstruction::Build({dest, src1, src2}, IRInstrs::Mult);
	i.is_unsigned = false;
	i.is_mmi_divmul = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitDIVU1(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("divu1 %s, %s\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);

	src1.SetReg(rt);
	src2.SetReg(rs);

	i = IRInstruction::Build({src1, src2}, IRInstrs::Div);
	i.is_unsigned = true;
	i.is_mmi_divmul = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitPADDSB(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src1.SetReg(rs);
	src2.SetReg(rt);

	i = IRInstruction::Build({dest, src1, src2}, IRInstrs::PADDSB);

	cur_block->ir.push_back(i);
}

void JIT::EmitPSUBB(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src1.SetReg(rs);
	src2.SetReg(rt);

	i = IRInstruction::Build({dest, src1, src2}, IRInstrs::PSUBB);

	cur_block->ir.push_back(i);
}

void JIT::EmitPMFHI(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;

	IRValue dst = IRValue(IRValue::Reg);

	dst.SetReg(rd);
	i = IRInstruction::Build({dst}, IRInstrs::PMFHI);

	cur_block->ir.push_back(i);
}

void JIT::EmitPMFLO(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;

	IRValue dst = IRValue(IRValue::Reg);

	dst.SetReg(rd);
	i = IRInstruction::Build({dst}, IRInstrs::PMFLO);

	cur_block->ir.push_back(i);
}

void JIT::EmitPCPYLD(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src1.SetReg(rt);
	src2.SetReg(rs);

	i = IRInstruction::Build({src2, src1, dest}, IRInstrs::PCPYLD);

	cur_block->ir.push_back(i);
}

void JIT::EmitPAND(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src1.SetReg(rt);
	src2.SetReg(rs);

	i = IRInstruction::Build({dest, src1, src2}, IRInstrs::PAND);

	cur_block->ir.push_back(i);
}

void JIT::EmitPCPYUD(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src1.SetReg(rt);
	src2.SetReg(rs);

	i = IRInstruction::Build({src2, src1, dest}, IRInstrs::PCPYUD);

	cur_block->ir.push_back(i);
}

void JIT::EmitPOR(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src1.SetReg(rt);
	src2.SetReg(rs);

	i = IRInstruction::Build({dest, src1, src2}, IRInstrs::POR);

	cur_block->ir.push_back(i);
}

void JIT::EmitPNOR(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src1.SetReg(rt);
	src2.SetReg(rs);

	i = IRInstruction::Build({dest, src1, src2}, IRInstrs::PNOR);

	cur_block->ir.push_back(i);
}

void JIT::EmitPCPYH(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src1 = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src1.SetReg(rt);

	i = IRInstruction::Build({dest, src1}, IRInstrs::PCPYH);

	cur_block->ir.push_back(i);
}

void JIT::EmitPADDUW(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src1.SetReg(rt);
	src2.SetReg(rs);

	i = IRInstruction::Build({dest, src1, src2}, IRInstrs::PADDUSW);

	cur_block->ir.push_back(i);
}

void JIT::EmitMFC0(uint32_t instr, EE_JIT::IRInstruction& i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("mfc0 %s, r%d (0x%08x)\n", EmotionEngine::Reg(rt), rd, EmotionEngine::state.cop0_regs[rd]);

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

	if (EmotionEngine::can_disassemble)
		printf("mtc0 %s, r%d\n", EmotionEngine::Reg(rt), rd);

	IRValue dest = IRValue(IRValue::Cop0Reg);
	IRValue src = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src.SetReg(rt);

	i = IRInstruction::Build({dest, src}, IRInstrs::MOV);

	cur_block->ir.push_back(i);
}

void JIT::EmitERET(uint32_t instr, EE_JIT::IRInstruction &i)
{
	if (EmotionEngine::can_disassemble)
		printf("ERET: Returning to 0x%08x\n", EmotionEngine::state.cop0_regs[14]);

	i = IRInstruction::Build({}, IRInstrs::ERET);

	cur_block->ir.push_back(i);
}

void JIT::EmitMFC1(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("mfc1 %s, r%d (0x%08x)\n", EmotionEngine::Reg(rt), rd, EmotionEngine::state.cop0_regs[rd]);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Cop1Reg);

	dest.SetReg(rt);
	src.SetReg(rd);

	i = IRInstruction::Build({dest, src}, IRInstrs::MOV);

	cur_block->ir.push_back(i);
}

void JIT::EmitMTC1(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;

	if (EmotionEngine::can_disassemble)
		printf("mtc1 %s, r%d\n", EmotionEngine::Reg(rt), rd);

	IRValue dest = IRValue(IRValue::Cop1Reg);
	IRValue src = IRValue(IRValue::Reg);

	dest.SetReg(rd);
	src.SetReg(rt);

	i = IRInstruction::Build({dest, src}, IRInstrs::MOV);

	cur_block->ir.push_back(i);
}

void JIT::EmitBC1F(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int32_t i_ = (int32_t)(int16_t)(instr & 0xffff) << 2;

	IRValue imm = IRValue(IRValue::Imm);
	imm.SetImm32(i_);

	i = IRInstruction::Build({imm}, IRInstrs::BC1F);

	cur_block->ir.push_back(i);
}

void JIT::EmitBC1T(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int32_t i_ = (int32_t)(int16_t)(instr & 0xffff) << 2;

	IRValue imm = IRValue(IRValue::Imm);
	imm.SetImm32(i_);

	i = IRInstruction::Build({imm}, IRInstrs::BC1T);

	cur_block->ir.push_back(i);
}

void JIT::EmitADDS(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::ADDS);

	cur_block->ir.push_back(i);
}

void JIT::EmitSUBS(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::SUBS);

	cur_block->ir.push_back(i);
}

void JIT::EmitMULS(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::MULS);

	cur_block->ir.push_back(i);
}

void JIT::EmitDIVS(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::DIVS);

	cur_block->ir.push_back(i);
}

void JIT::EmitMOVS(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::MOVS);

	cur_block->ir.push_back(i);
}

void JIT::EmitNEGS(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::NEGS);

	cur_block->ir.push_back(i);
}

void JIT::EmitADDAS(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::ADDAS);

	cur_block->ir.push_back(i);
}

void JIT::EmitMADDS(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::MADDS);

	cur_block->ir.push_back(i);
}

void JIT::EmitCVTW(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::CVTW);

	cur_block->ir.push_back(i);
}

void JIT::EmitCEQS(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::CEQS);

	cur_block->ir.push_back(i);
}

void JIT::EmitCLES(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::CLES);

	cur_block->ir.push_back(i);
}

void JIT::EmitCVTS(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::CVTS);

	cur_block->ir.push_back(i);
}

void JIT::EmitQMFC2(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 11) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);

	dest.SetReg(rt);
	src.SetReg(rs);

	i = IRInstruction::Build({dest, src}, IRInstrs::QMFC2);

	cur_block->ir.push_back(i);
}

void JIT::EmitCFC2(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 11) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);

	dest.SetReg(rt);
	src.SetReg(rs);

	i = IRInstruction::Build({dest, src}, IRInstrs::CFC2);

	cur_block->ir.push_back(i);
}

void JIT::EmitQMTC2(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 11) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);

	dest.SetReg(rt);
	src.SetReg(rs);

	i = IRInstruction::Build({dest, src}, IRInstrs::QMTC2);

	cur_block->ir.push_back(i);
}

void JIT::EmitCTC2(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 11) & 0x1F;

	IRValue dest = IRValue(IRValue::Reg);
	IRValue src = IRValue(IRValue::Reg);

	dest.SetReg(rt);
	src.SetReg(rs);

	i = IRInstruction::Build({dest, src}, IRInstrs::CTC2);

	cur_block->ir.push_back(i);
}

void JIT::EmitVMADDZ(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::VMADDZ);

	cur_block->ir.push_back(i);
}

void JIT::EmitVMADDW(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::VMADDW);

	cur_block->ir.push_back(i);
}

void JIT::EmitVSUB(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::VSUB);

	cur_block->ir.push_back(i);
}

void JIT::EmitVIADD(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::VIADD);

	cur_block->ir.push_back(i);
}

void JIT::EmitVMADDAX(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::VMADDAX);

	cur_block->ir.push_back(i);
}

void JIT::EmitVMADDAY(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::VMADDAY);

	cur_block->ir.push_back(i);
}

void JIT::EmitVMADDAZ(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::VMADDAZ);

	cur_block->ir.push_back(i);
}

void JIT::EmitVMULAX(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::VMULAX);

	cur_block->ir.push_back(i);
}

void JIT::EmitVMULAW(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::VMULAW);

	cur_block->ir.push_back(i);
}

void JIT::EmitVCLIPW(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::VCLIPW);

	cur_block->ir.push_back(i);
}

void JIT::EmitVSQI(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::VSQI);

	cur_block->ir.push_back(i);
}

void JIT::EmitVISWR(uint32_t instr, EE_JIT::IRInstruction &i)
{
	IRValue inst = IRValue(IRValue::Imm);

	inst.SetImm32Unsigned(instr);

	i = IRInstruction::Build({inst}, IRInstrs::VISWR);

	cur_block->ir.push_back(i);
}

void JIT::EmitBC0F(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int32_t i_ = (int32_t)(int16_t)(instr & 0xffff) << 2;

	IRValue imm = IRValue(IRValue::Imm);
	imm.SetImm32(i_);

	i = IRInstruction::Build({imm}, IRInstrs::BC0F);

	cur_block->ir.push_back(i);
}

void JIT::EmitBC0T(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int32_t i_ = (int32_t)(int16_t)(instr & 0xffff) << 2;

	IRValue imm = IRValue(IRValue::Imm);
	imm.SetImm32(i_);

	i = IRInstruction::Build({imm}, IRInstrs::BC0T);

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

	if (!cur_block->ir.size())
		EmotionEngine::GetState()->pc_at = EmotionEngine::GetState()->pc;
	else
		EmotionEngine::GetState()->pc_at += 4;

	if (instr == 0)
	{
		if (EmotionEngine::can_disassemble)
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
		case 0x02:
			EmitSRL(instr, current_instruction);
			break;
		case 0x03:
			EmitSRA(instr, current_instruction);
			break;
		case 0x04:
			EmitSLLV(instr, current_instruction);
			break;
		case 0x06:
			EmitSRLV(instr, current_instruction);
			break;
		case 0x07:
			EmitSRAV(instr, current_instruction);
			break;
		case 0x08:
			EmitJR(instr, current_instruction);
			break;
		case 0x09:
			EmitJALR(instr, current_instruction);
			break;
		case 0x0A:
			EmitMOVZ(instr, current_instruction);
			break;
		case 0x0B:
			EmitMOVN(instr, current_instruction);
			break;
		case 0x0C:
			EmitSyscall(instr, current_instruction);
			break;
		case 0x0D:
			EmitBreak(instr, current_instruction);
			break;
		case 0x0F:
			// printf("sync\n");
			break;
		case 0x10:
			EmitMFHI(instr, current_instruction);
			break;
		case 0x11:
			EmitMTHI(instr, current_instruction);
			break;
		case 0x12:
			EmitMFLO(instr, current_instruction);
			break;
		case 0x13:
			EmitMTLO(instr, current_instruction);
			break;
		case 0x14:
			EmitDSLLV(instr, current_instruction);
			break;
		case 0x16:
			EmitDSRLV(instr, current_instruction);
			break;
		case 0x17:
			EmitDSRAV(instr, current_instruction);
			break;
		case 0x18:
			EmitMULT(instr, current_instruction);
			break;
		case 0x19:
			EmitMULTU(instr, current_instruction);
			break;
		case 0x1A:
			EmitDIV(instr, current_instruction);
			break;
		case 0x1B:
			EmitDIVU(instr, current_instruction);
			break;
		case 0x20:
			EmitADD(instr, current_instruction);
			break;
		case 0x21:
			EmitADDU(instr, current_instruction);
			break;
		case 0x22:
			EmitSUB(instr, current_instruction);
			break;
		case 0x23:
			EmitSUBU(instr, current_instruction);
			break;
		case 0x24:
			EmitAND(instr, current_instruction);
			break;
		case 0x25:
			EmitOR(instr, current_instruction);
			break;
		case 0x26:
			EmitXOR(instr, current_instruction);
			break;
		case 0x27:
			EmitNOR(instr, current_instruction);
			break;
		case 0x28:
		case 0x29:
			break;
		case 0x2A:
			EmitSLT(instr, current_instruction);
			break;
		case 0x2B:
			EmitSLTU(instr, current_instruction);
			break;
		case 0x2D:
			EmitDADDU(instr, current_instruction);
			break;
		case 0x2F:
			EmitDSUBU(instr, current_instruction);
			break;
		case 0x34:
			break;
		case 0x38:
			EmitDSLL(instr, current_instruction);
			break;
		case 0x3A:
			EmitDSRL(instr, current_instruction);
			break;
		case 0x3C:
			EmitDSLL32(instr, current_instruction);
			break;
		case 0x3E:
			EmitDSRL32(instr, current_instruction);
			break;
		case 0x3F:
			EmitDSRA32(instr, current_instruction);
			break;
		default:
			printf("[emu/CPU]: Cannot emit unknown special instruction 0x%02x (0x%08x)\n", opcode, instr);
			cur_block->ir.clear();
			delete cur_block;
			exit(1);
		}
		break;
	}
	case 0x01:
	{
		opcode = (instr >> 16) & 0x1F;
		switch (opcode)
		{
		case 0x00:
			EmitBLTZ(instr, current_instruction);
			break;
		case 0x01:
			EmitBGEZ(instr, current_instruction);
			break;
		case 0x02:
			EmitBLTZL(instr, current_instruction);
			break;
		case 0x03:
			EmitBGEZL(instr, current_instruction);
			break;
		case 0x0c:
			break;
		case 0x11:
			EmitBGEZAL(instr, current_instruction);
			break;
		default:
			printf("Unknown regimm opcode 0x%02x (0x0%08x)\n", opcode, instr);
			exit(1);
		}
		break;
	}
	case 0x02:
		EmitJ(instr, current_instruction);
		break;
	case 0x03:
		EmitJAL(instr, current_instruction);
		break;
	case 0x04:
		EmitBEQ(instr, current_instruction);
		break;
	case 0x05:
		EmitBNE(instr, current_instruction);
		break;
	case 0x06:
		EmitBLEZ(instr, current_instruction);
		break;
	case 0x07:
		EmitBGTZ(instr, current_instruction);
		break;
	case 0x08:
		EmitADDI(instr, current_instruction);
		break;
	case 0x09:
		EmitADDIU(instr, current_instruction);
		break;
	case 0x0A:
		EmitSLTI(instr, current_instruction);
		break;
	case 0x0B:
		EmitSLTIU(instr, current_instruction);
		break;
	case 0x0C:
		EmitANDI(instr, current_instruction);
		break;
	case 0x0D:
		EmitORI(instr, current_instruction);
		break;
	case 0x0E:
		EmitXORI(instr, current_instruction);
		break;
	case 0x0F:
		EmitLUI(instr, current_instruction);
		break;
	case 0x10:
		EmitCOP0(instr, current_instruction);
		break;
	case 0x11:
	{
		opcode = (instr >> 21) & 0x1F;
		switch (opcode)
		{
		case 0x00:
			EmitMFC1(instr, current_instruction);
			break;
		case 0x02:
			break;
		case 0x04:
			EmitMTC1(instr, current_instruction);
			break;
		case 0x06:
			break;
		case 0x08:
		{
			opcode = (instr >> 16) & 0x1F;
			switch (opcode)
			{
			case 0x00:
				EmitBC1F(instr, current_instruction);
				break;
			case 0x01:
				EmitBC1T(instr, current_instruction);
				break;
			default:
				printf("[emu/EE]: Unknown cop1 bc command 0x%02x (0x%08x)\n", opcode, instr);
				exit(1);
			}
			break;
		}
		case 0x10:
		{
			opcode = instr & 0x3F;
			switch (opcode)
			{
			case 0x00:
				EmitADDS(instr, current_instruction);
				break;
			case 0x01:
				EmitSUBS(instr, current_instruction);
				break;
			case 0x02:
				EmitMULS(instr, current_instruction);
				break;
			case 0x03:
				EmitDIVS(instr, current_instruction);
				break;
			case 0x06:
				EmitMOVS(instr, current_instruction);
				break;
			case 0x07:
				EmitNEGS(instr, current_instruction);
				break;
			case 0x18:
				EmitADDAS(instr, current_instruction);
				break;
			case 0x1c:
				EmitMADDS(instr, current_instruction);
				break;
			case 0x24:
				EmitCVTW(instr, current_instruction);
				break;
			case 0x32:
				EmitCEQS(instr, current_instruction);
				break;
			case 0x36:
				EmitCLES(instr, current_instruction);
				break;
			default:
				printf("[emu/EE]: Unknown cop1.s command 0x%02x (0x%08x)\n", opcode, instr);
				exit(1);
			}
			break;
		}
		case 0x14:
		{
			opcode = instr & 0x3F;
			switch (opcode)
			{
			case 0x20:
				EmitCVTS(instr, current_instruction);
				break;
			default:
				printf("[emu/EE]: Unknown cop1.w command 0x%02x (0x%08x)\n", opcode, instr);
				exit(1);
			}
			break;
		}
		default:
			printf("[emu/EE]: Unknown cop1 command 0x%02x (0x%08x)\n", opcode, instr);
			exit(1);
		}
		break;
	}
	case 0x12:
	{
		opcode = (instr >> 21) & 0x1F;
		switch (opcode)
		{
		case 0x01:
			EmitQMFC2(instr, current_instruction);
			break;
		case 0x02:
			EmitCFC2(instr, current_instruction);
			break;
		case 0x05:
			EmitQMTC2(instr, current_instruction);
			break;
		case 0x06:
			EmitCTC2(instr, current_instruction);
			break;
		case 0x10 ... 0x1F:
		{
			opcode = instr & 0x3F;
			switch (opcode)
			{
			case 0x0a:
				EmitVMADDZ(instr, current_instruction);
				break;
			case 0x0b:
				EmitVMADDW(instr, current_instruction);
				break;
			case 0x2c:
				EmitVSUB(instr, current_instruction);
				break;
			case 0x30:
				EmitVIADD(instr, current_instruction);
				break;
			case 0x3c ... 0x3f:
			{
				opcode = (instr & 0x3) | ((instr >> 6) & 0x1F) << 2;
				switch (opcode)
				{
				case 0x08:
					EmitVMADDAX(instr, current_instruction);
					break;
				case 0x09:
					EmitVMADDAY(instr, current_instruction);
					break;
				case 0x0a:
					EmitVMADDAZ(instr, current_instruction);
					break;
				case 0x18:
					EmitVMULAX(instr, current_instruction);
					break;
				case 0x1B:
					EmitVMULAW(instr, current_instruction);
					break;
				case 0x1F:
					EmitVCLIPW(instr, current_instruction);
					break;
				case 0x35:
					EmitVSQI(instr, current_instruction);
					break;
				case 0x3f:
					EmitVISWR(instr, current_instruction);
					break;
				default:
					printf("[emu/CPU]: Cannot emit unknown cop2 special2 instruction 0x%02x (0x%08x)\n", opcode, instr);
					exit(1);
				}
				break;
			}
			default:
				printf("[emu/CPU]: Cannot emit unknown cop2 special1 instruction 0x%02x (0x%08x)\n", opcode, instr);
				exit(1);
			}
			break;
		}
		default:
			printf("[emu/CPU]: Cannot emit unknown cop2 instruction 0x%02x (0x%08x)\n", opcode, instr);
			exit(1);
		}
		break;
	}
	case 0x14:
		EmitBEQL(instr, current_instruction);
		break;
	case 0x15:
		EmitBNEL(instr, current_instruction);
		break;
	case 0x19:
		EmitDADDIU(instr, current_instruction);
		break;
	case 0x1A:
		EmitLDL(instr, current_instruction);
		break;
	case 0x1B:
		EmitLDR(instr, current_instruction);
		break;
	case 0x1C:
	{
		opcode = instr & 0x3F;
		switch (opcode)
		{
		case 0x00:
			EmitMADD(instr, current_instruction);
			break;
		case 0x04:
			EmitPLZCW(instr, current_instruction);
			break;
		case 0x08:
		{
			opcode = (instr >> 6) & 0x1F;

			switch (opcode)
			{
			case 0x09:
				EmitPSUBB(instr, current_instruction);
				break;
			default:
				printf("[emu/CPU]: Cannot emit unknown mmi0 instruction 0x%02x\n", opcode);
				delete cur_block;
				exit(1);
			}
			break;
		}
		case 0x09:
		{
			opcode = (instr >> 6) & 0x1F;

			switch (opcode)
			{
			case 0x08:
				EmitPMFHI(instr, current_instruction);
				break;
			case 0x09:
				EmitPMFLO(instr, current_instruction);
				break;
			case 0x0E:
				EmitPCPYLD(instr, current_instruction);
				break;
			case 0x12:
				EmitPAND(instr, current_instruction);
				break;
			default:
				printf("[emu/CPU]: Cannot emit unknown mmi2 instruction 0x%02x\n", opcode);
				delete cur_block;
				exit(1);
			}
			break;
		}
		case 0x10:
			EmitMFHI1(instr, current_instruction);
			break;
		case 0x11:
			EmitMTHI1(instr, current_instruction);
			break;
		case 0x12:
			EmitMFLO1(instr, current_instruction);
			break;
		case 0x13:
			EmitMTLO1(instr, current_instruction);
			break;
		case 0x18:
			EmitMULT1(instr, current_instruction);
			break;
		case 0x1B:
			EmitDIVU1(instr, current_instruction);
			break;
		case 0x28:
		{
			opcode = (instr >> 6) & 0x1F;

			switch (opcode)
			{
			case 0x10:
				EmitPADDUW(instr, current_instruction);
				break;
			default:
				printf("[emu/CPU]: Cannot emit unknown mmi1 instruction 0x%02x (0x%08x)\n", opcode, instr);
				delete cur_block;
				exit(1);
			}
			break;
		}
		case 0x29:
		{
			opcode = (instr >> 6) & 0x1F;

			switch (opcode)
			{
			case 0x0e:
				EmitPCPYUD(instr, current_instruction);
				break;
			case 0x12:
				EmitPOR(instr, current_instruction);
				break;
			case 0x13:
				EmitPNOR(instr, current_instruction);
				break;
			case 0x1b:
				EmitPCPYH(instr, current_instruction);
				break;
			default:
				printf("[emu/CPU]: Cannot emit unknown mmi3 instruction 0x%02x\n", opcode);
				delete cur_block;
				exit(1);
			}
			break;
		}
		case 0x30:
			break;
		default:
			printf("[emu/CPU]: Cannot emit unknown mmi instruction 0x%02x (0x%08x)\n", opcode, instr);
			delete cur_block;
			exit(1);
		}
		break;
	}
	case 0x1E:
		EmitLQ(instr, current_instruction);
		break;
	case 0x1F:
		EmitSQ(instr, current_instruction);
		break;
	case 0x20:
		EmitLB(instr, current_instruction);
		break;
	case 0x21:
		EmitLH(instr, current_instruction);
		break;
	case 0x23:
		EmitLW(instr, current_instruction);
		break;
	case 0x24:
		EmitLBU(instr, current_instruction);
		break;
	case 0x25:
		EmitLHU(instr, current_instruction);
		break;
	case 0x27:
		EmitLWU(instr, current_instruction);
		break;
	case 0x28:
		EmitSB(instr, current_instruction);
		break;
	case 0x29:
		EmitSH(instr, current_instruction);
		break;
	case 0x2b:
		EmitSW(instr, current_instruction);
		break;
	case 0x2c:
		EmitSDL(instr, current_instruction);
		break;
	case 0x2d:
		EmitSDR(instr, current_instruction);
		break;
	case 0x2f:
		// printf("cache\n");
		break;
	case 0x31:
		EmitLWC1(instr, current_instruction);
		break;
	case 0x36:
		EmitLQC2(instr, current_instruction);
		break;
	case 0x37:
		EmitLD(instr, current_instruction);
		break;
	case 0x39:
		EmitSWC1(instr, current_instruction);
		break;
	case 0x3e:
		EmitSQC2(instr, current_instruction);
		break;
	case 0x3f:
		EmitSD(instr, current_instruction);
		break;
	default:
		printf("[emu/CPU]: Cannot emit unknown instruction 0x%02x (0x%08x)\n", opcode, instr);
		delete cur_block;
		exit(1);
	}
}

void JIT::EmitPrequel(uint32_t guest_start)
{
	// printf("---------------------\n");
	// printf("0x%08x:\n", EmotionEngine::state.pc);
	cur_block = new Block;
	cur_block->guest_addr = guest_start;
	cur_block->entry = reinterpret_cast<uint8_t*>(emit->GetFreeBase());
}

JIT::EntryFunc JIT::EmitDone(size_t cycles_taken)
{
	// printf("Done.\n");

	IRValue cycles = IRValue(IRValue::Imm);
	cycles.SetImm32Unsigned(cycles_taken);

	auto i = IRInstruction::Build({cycles}, IRInstrs::UpdateCopCount);
	cur_block->ir.push_back(i);

	blockCache[cur_block->guest_addr] = cur_block;

	EE_JIT::emit->TranslateBlock(cur_block);

	cur_block->cycles = cycles_taken;

	return (EntryFunc)cur_block->entry;
}

Block* JIT::GetExistingBlock(uint32_t start)
{
	return blockCache[start];
}

bool JIT::DoesBlockExist(uint32_t addr)
{
	return blockCache.find(addr) != blockCache.end();
}

void JIT::CheckCacheFull()
{
	if (blockCache.size() >= 75)
	{
		emit->ResetFreeBase();
		// for (auto b : blockCache)
		// 	delete b.second;
		blockCache.clear();
	}
}

void JIT::MarkDirty(uint32_t address, uint32_t size)
{
	for (auto i = address; i < address+size; i++)
	{
		if (blockCache.find(size) != blockCache.end())
		{
			delete blockCache[i];
			blockCache[i] = nullptr;
		}
	}
}

}  // namespace EE_JIT

EE_JIT::JIT jit;

namespace EmotionEngine
{

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
		case 0x0C:
			return true;
		default:
			return false;
		}
		break;
	}
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x14:
	case 0x15:
		return true;
	case 0x10:
		opcode = (instr >> 21) & 0x3F;
		switch (opcode)
		{
		case 0x10:
		{
			opcode = instr & 0x3F;
			switch (opcode)
			{
			case 0x18:
				return true;
			default:
				return false;
			}
		}
		default:
			return false;
		}
		break;
	case 0x11:
		opcode = (instr >> 21) & 0x1F;
		switch (opcode)
		{
		case 0x8:
			return true;
		}
		break;
	default:
		return false;
	}

	return false;
}

EE_JIT::JIT::EntryFunc EmitDone(uint64_t cycles_taken)
{
	return jit.EmitDone(cycles_taken);
}

EE_JIT::JIT::EntryFunc GetExistingBlockFunc(uint32_t addr)
{
	return (EE_JIT::JIT::EntryFunc)jit.GetExistingBlock(addr)->entry;
}

uint64_t GetExistingBlockCycles(uint32_t addr)
{
	return jit.GetExistingBlock(addr)->cycles;
}

#define BLOCKCACHE_ENABLE

int Clock()
{
// 	jit.CheckCacheFull();

// #ifdef BLOCKCACHE_ENABLE
// 	if (!jit.DoesBlockExist(state.pc))
// 	{
// #endif
// 		jit.EmitPrequel(state.pc);

// 		bool isBranch = false;
// 		bool isBranchDelayed = false;

// 		uint32_t pc = state.pc;
// 		uint32_t next_pc = state.next_pc;

// 		int cycles = 0;
// 		int instrs_emitted = 0;

// 		do
// 		{
// 			instrs_emitted++;
// 			uint32_t instr = Bus::Read32(pc);
// 			pc = next_pc;
// 			next_pc += 4;

// 			state.pc_at = pc - 4;

// 			// printf("0x%08x (0x%08x): ", pc - 4, instr);

// 			jit.EmitIR(instr);

// 			isBranch = isBranchDelayed;
// 			isBranchDelayed = IsBranch(instr);

// 			if (instrs_emitted == 20 && !isBranchDelayed)
// 				break;

// 			cycles += 1;
// 		} while (!isBranch);

// 		auto func = jit.EmitDone(cycles);
// 		func();

// 		return cycles;
// #ifdef BLOCKCACHE_ENABLE
// 	}
// 	else
// 	{
// 		auto b = jit.GetExistingBlock(state.pc);

// 		if (!b)
// 		{
// 		 	// printf("Something has gone seriously wrong!\n");
// 		 	exit(1);
// 		}

// 		auto func = (EE_JIT::JIT::EntryFunc)b->entry;
// 		func();

// 		return b->cycles;
// 	}
// #endif

	EE_JIT::emit->EnterDispatcher();
}

void Dump()
{
	EE_JIT::emit->Dump();

	for (int i = 0; i < 32; i++)
		printf("%s\t->\t0x%lx%016lx\n", Reg(i), state.regs[i].u64[1], state.regs[i].u64[0]);
	printf("pc\t->\t0x%08x\n", state.pc);
	printf("pc_at\t->\t0x%08x\n", state.pc_at);
	printf("hi\t->\t0x%08lx\n", state.hi);
	printf("lo\t->\t0x%08lx\n", state.lo);
	printf("hi1\t->\t0x%08lx\n", state.hi1);
	printf("lo1\t->\t0x%08lx\n", state.lo1);
	printf("status\t->\t0x%08x\n", state.cop0_regs[12]);
	printf("cause\t->\t0x%08x\n", state.cop0_regs[13]);
	for (int i = 0; i < 32; i++)
		printf("f%d\t->\t%f\n", i, convert(state.fprs[i].i));
}

ProcessorState* GetState()
{
	return &state;
}
void MarkDirty(uint32_t address, uint32_t size)
{
	jit.MarkDirty(address, size);
}

void EmitPrequel()
{
	jit.EmitPrequel(state.pc);
}

union COP0CAUSE
{
	uint32_t value;
	struct
	{
		uint32_t : 2;
		uint32_t excode : 5;	/* Exception Code */
		uint32_t : 3;
		uint32_t ip0_pending : 1;
		uint32_t ip1_pending : 1;
		uint32_t siop : 1;
		uint32_t : 2;
		uint32_t timer_ip_pending : 1;
		uint32_t exc2 : 3;
		uint32_t : 9;
		uint32_t ce : 2;
		uint32_t bd2 : 1;
		uint32_t bd : 1;
	};
};

union COP0Status
{
    uint32_t value;
    struct
    {
        uint32_t ie : 1; /* Interrupt Enable */
        uint32_t exl : 1; /* Exception Level */
        uint32_t erl : 1; /* Error Level */
        uint32_t ksu : 2; /* Kernel/Supervisor/User Mode bits */
        uint32_t : 5;
        uint32_t im0 : 1; /* Int[1:0] signals */
        uint32_t im1 : 1;
        uint32_t bem : 1; /* Bus Error Mask */
        uint32_t : 2;
        uint32_t im7 : 1; /* Internal timer interrupt  */
        uint32_t eie : 1; /* Enable IE */
        uint32_t edi : 1; /* EI/DI instruction Enable */
        uint32_t ch : 1; /* Cache Hit */
        uint32_t : 3;
        uint32_t bev : 1; /* Location of TLB refill */
        uint32_t dev : 1; /* Location of Performance counter */
        uint32_t : 2;
        uint32_t fr : 1; /* Additional floating point registers */
        uint32_t : 1;
        uint32_t cu : 4; /* Usability of each of the four coprocessors */
    };
};

void Exception(uint8_t code)
{
	static uint32_t exception_addr[2] = { 0x80000000, 0xBFC00200 };

	can_disassemble = false;

	if (code == 0x08)
	{
		int syscall_number = std::abs((int)EmotionEngine::state.regs[3].u32[0]);
		if (syscall_number != 122)
			printf("Syscall %d\n", syscall_number);
		if (syscall_number == 0x02)
		{
			printf("SetGsCrt(%d, %d, %d)\n", EmotionEngine::GetState()->regs[4].u32[0], EmotionEngine::GetState()->regs[5].u32[0], EmotionEngine::GetState()->regs[6].u32[0]);
		}
		else if (syscall_number == 0x77)
		{
			printf("sceSifSetDma(0x%08x, %d)\n", EmotionEngine::GetState()->regs[4].u32[0], EmotionEngine::GetState()->regs[5].u32[0]);
		}
		else if (syscall_number == 0x20)
		{
			printf("CreateThread(0x%08x)\n", EmotionEngine::GetState()->regs[4].u32[0]);
		}
		else if (syscall_number == 0x2F)
		{
			printf("GetThreadID()\n");
		}
		else if (syscall_number == 0x3C)
		{
			printf("InitMainThread(0x%08x, 0x%08x, %d, 0x%08x)\n", EmotionEngine::GetState()->regs[4].u32[0], EmotionEngine::GetState()->regs[5].u32[0], EmotionEngine::GetState()->regs[6].u32[0], EmotionEngine::GetState()->regs[7].u32[0]);
		}
		else if (syscall_number == 0x3D)
		{
			printf("InitHeap(0x%08x, %d)\n", EmotionEngine::GetState()->regs[4].u32[0], EmotionEngine::GetState()->regs[5].u32[0]);
		}
		else if (syscall_number == 0x3E)
		{
			printf("EndOfHeap()\n");
		}
		else if (syscall_number == 0x40)
		{
			printf("CreateSema(0x%08x)\n", EmotionEngine::GetState()->regs[4].u32[0]);
		}
		else if (syscall_number == 0x42)
		{
			printf("SignalSema(%d)\n", EmotionEngine::GetState()->regs[4].u32[0]);
		}
		else if (syscall_number == 0x44)
		{
			printf("WaitSema(%d)\n", EmotionEngine::GetState()->regs[4].u32[0]);
		}
		else if (syscall_number == 0x64)
		{
			printf("FlushCache(%d)\n", EmotionEngine::GetState()->regs[4].u32[0]);
		}
		else if (syscall_number == 121)
		{
			printf("sceSifSetReg(0x%08x, 0x%08x)\n", EmotionEngine::GetState()->regs[4].u32[0], EmotionEngine::GetState()->regs[5].u32[0]);
		}
		else if (syscall_number == 5)
		{
			printf("_ExceptionEpilogue()\n");
		}
	}

	COP0CAUSE cause;
	COP0Status status;
	status.value = state.cop0_regs[12];
	cause.value = state.cop0_regs[13];

	cause.excode = code;

	uint32_t vector = 0x180;

	if (!status.exl)
	{
		state.cop0_regs[14] = state.pc-4;

		switch (code)
		{
		case 0:
			vector = 0x200;
			break;
		default:
			vector = 0x180;
			break;
		}

		status.exl = true;
	}

	state.pc = exception_addr[status.bev] + vector;
	state.next_pc = state.pc + 4;

	state.cop0_regs[12] = status.value;
	state.cop0_regs[13] = cause.value;
}

void SetIp1Pending()
{
	COP0CAUSE cause;
	cause.value = EmotionEngine::GetState()->cop0_regs[13];
	cause.ip1_pending = true;
	EmotionEngine::GetState()->cop0_regs[13] = cause.value;
}

void ClearIp1Pending()
{
	COP0CAUSE cause;
	cause.value = EmotionEngine::GetState()->cop0_regs[13];
	cause.ip1_pending = false;
	EmotionEngine::GetState()->cop0_regs[13] = cause.value;
}

void SetIp0Pending()
{
	COP0CAUSE cause;
	cause.value = EmotionEngine::GetState()->cop0_regs[13];
	cause.ip0_pending = true;
	EmotionEngine::GetState()->cop0_regs[13] = cause.value;
}

void CheckCacheFull()
{
	jit.CheckCacheFull();
}
bool DoesBlockExit(uint32_t addr)
{
	return jit.DoesBlockExist(addr);
}
void EmitIR(uint32_t instr)
{
	jit.EmitIR(instr);
}

}  // namespace EmotionEngine
