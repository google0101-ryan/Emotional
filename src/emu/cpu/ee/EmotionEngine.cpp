// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include "EmotionEngine.h"
#include <emu/memory/Bus.h>
#include <emu/cpu/ee/x64/emit.h>

namespace EmotionEngine
{
	ProcessorState state;
}

namespace EE_JIT
{

void JIT::EmitJ(uint32_t instr, EE_JIT::IRInstruction &i)
{
	// printf("j 0x%08x\n", (instr & 0x3ffffff) << 2);

	uint32_t target_address = (instr & 0x3FFFFFF) << 2;

	IRValue imm = IRValue(IRValue::Imm);
	imm.SetImm32Unsigned(target_address);

	i = IRInstruction::Build({imm}, IRInstrs::JumpImm);
	i.should_link = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitJAL(uint32_t instr, EE_JIT::IRInstruction &i)
{
	// printf("jal 0x%08x\n", (instr & 0x3ffffff) << 2);

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
	
	// printf("beq %s, %s, 0x%08x (%x)\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_, i_);

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

	// printf("bne %s, %s, 0x%08x\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_);

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
	
	// printf("blez %s, 0x%08x (%x)\n", EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_, i_);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);

	dest.SetReg(rs);
	imm.SetImm32((int32_t)(int16_t)(instr & 0xffff) << 2);

	i = IRInstruction::Build({dest, imm}, IRInstrs::BranchRegImm);
	i.b_type = IRInstruction::BranchType::LE;

	cur_block->ir.push_back(i);
}

void JIT::EmitBGTZ(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rs = (instr >> 21) & 0x1F;

	int32_t i_ = (int32_t)(int16_t)(instr & 0xffff) << 2;
	
	// printf("bgtz %s, 0x%08x (%x)\n", EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_, i_);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);

	dest.SetReg(rs);
	imm.SetImm32((int32_t)(int16_t)(instr & 0xffff) << 2);

	i = IRInstruction::Build({dest, imm}, IRInstrs::BranchRegImm);
	i.b_type = IRInstruction::BranchType::GT;

	cur_block->ir.push_back(i);
}

void JIT::EmitADDIU(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	// printf("addiu %s, %s, %d\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), (int32_t)(int16_t)(instr & 0xffff));

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
	
	// printf("slti %s, %s, 0x%02x\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), (int32_t)(int16_t)(instr & 0xffff));

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
	
	// printf("sltiu %s, %s, 0x%04x\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), i_);

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

	// printf("andi %s, %s, 0x%04x (0x%08x)\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), instr & 0xffff, EmotionEngine::state.regs[rs].u32[0]);

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

	// printf("ori %s, %s, 0x%04x\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), instr & 0xffff);

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

	// printf("xori %s, %s, 0x%04x\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), instr & 0xffff);

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

	// printf("lui %s, 0x%04x\n", EmotionEngine::Reg(rt), instr & 0xffff);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);
	dest.SetReg(rt);
	imm.SetImm32(((int32_t)(int16_t)(instr & 0xffff)) << 16);

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
		// printf("tlbwi\n");
		break;
	default:
		// printf("[emu/CPU]: Cannot emit unknown cop0 instruction 0x%02x\n", op);
		exit(1);
	}
}

void JIT::EmitBEQL(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;
	int32_t i_ = (int32_t)(int16_t)(instr & 0xffff) << 2;

	// printf("beql %s, %s, 0x%08x\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_);

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

	// printf("bnel %s, %s, 0x%08x\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_);

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

	// printf("daddiu %s, %s, %d\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs), (int32_t)(int16_t)(instr & 0xffff));

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

	// printf("ldl %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::LDL);
	
	cur_block->ir.push_back(i);
}

void JIT::EmitLDR(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	// printf("ldr %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::LDR);
	
	cur_block->ir.push_back(i);
}

void JIT::EmitLQ(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);

	// printf("lq %s, %d(%s) (0x%08x)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base), EmotionEngine::state.regs[_base].u32[0] + _offset);

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
	
	// printf("sq %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

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

	// printf("lb %s, %d(%s) (0x%08x)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base), EmotionEngine::state.regs[_base].u32[0] + _offset);

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

	// printf("lh %s, %d(%s) (0x%08x)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base), EmotionEngine::state.regs[_base].u32[0] + _offset);

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

	// printf("lw %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

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

	// printf("lbu %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

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

	// printf("lhu %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

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

	// printf("lwu %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

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
	
	// printf("sb %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

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
	
	// printf("sh %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

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
	
	// printf("sw %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

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
	
	// printf("sdl %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::SDL);
	
	cur_block->ir.push_back(i);
}

void JIT::EmitSDR(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);
	
	// printf("sdr %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

	IRValue base = IRValue(IRValue::Reg);
	IRValue rt = IRValue(IRValue::Reg);
	IRValue offset = IRValue(IRValue::Imm);

	base.SetReg(_base);
	rt.SetReg(_rt);
	offset.SetImm(_offset);

	i = IRInstruction::Build({rt, offset, base}, IRInstrs::SDR);
	
	cur_block->ir.push_back(i);
}

void JIT::EmitLD(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);
	
	// printf("ld %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

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

	// printf("swc1 %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

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

void JIT::EmitSD(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int _rt = (instr >> 16) & 0x1F;
	int _base = (instr >> 21) & 0x1F;
	int32_t _offset = (int16_t)(instr & 0xffff);
	
	// printf("sd %s, %d(%s)\n", EmotionEngine::Reg(_rt), _offset, EmotionEngine::Reg(_base));

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

	// printf("sll %s, %s, %d\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), sa);

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

	// printf("srl %s, %s, %d\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), sa);

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

	// printf("sra %s, %s, %d\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), sa);

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

	// printf("sllv %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

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

void JIT::EmitSRAV(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	// printf("srav %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

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

	// printf("jr %s\n", EmotionEngine::Reg(rs));

	IRValue src = IRValue(IRValue::Reg);
	src.SetReg(rs);

	i = IRInstruction::Build({src}, IRInstrs::JumpAlways);

	cur_block->ir.push_back(i);
}

void JIT::EmitJALR(uint32_t instr, EE_JIT::IRInstruction &i)
{
	// printf("jalr\n");

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

void JIT::EmitMOVZ(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	// printf("movz %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

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

	// printf("movn %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

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

void JIT::EmitBreak(uint32_t instr, EE_JIT::IRInstruction &i)
{
	// printf("break\n");

	i = IRInstruction::Build({}, IRInstrs::UhOh);
	cur_block->ir.push_back(i);
}

void JIT::EmitMFHI(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;

	// printf("mfhi %s\n", EmotionEngine::Reg(rd));

	IRValue reg = IRValue(IRValue::Reg);
	reg.SetReg(rd);

	i = IRInstruction::Build({reg}, IRInstrs::MoveFromHi);
	cur_block->ir.push_back(i);	
}

void JIT::EmitMFLO(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;

	// printf("mflo %s\n", EmotionEngine::Reg(rd));

	IRValue reg = IRValue(IRValue::Reg);
	reg.SetReg(rd);

	i = IRInstruction::Build({reg}, IRInstrs::MoveFromLo);
	i.is_mmi_divmul = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitDSLLV(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	// printf("dsllv %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

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

void JIT::EmitDSRAV(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	// printf("dsrav %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

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

	// printf("mult %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

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

void JIT::EmitDIV(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	// printf("div %s, %s\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

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

	// printf("divu %s, %s\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);
	
	src1.SetReg(rt);
	src2.SetReg(rs);

	i = IRInstruction::Build({src1, src2}, IRInstrs::Div);
	i.is_unsigned = true;
	i.is_mmi_divmul = false;

	cur_block->ir.push_back(i);
}

void JIT::EmitADDU(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	// printf("addu %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue source2 = IRValue(IRValue::Reg);
	source.SetReg(rs);
	dest.SetReg(rd);
	source2.SetReg(rt);

	i = IRInstruction::Build({dest, source, source2}, IRInstrs::Add);

	cur_block->ir.push_back(i);
}

void JIT::EmitSUBU(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	// printf("subu %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

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

	// printf("and %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

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

	// printf("or %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

	IRValue dest = IRValue(IRValue::Reg);
	IRValue source = IRValue(IRValue::Reg);
	IRValue source2 = IRValue(IRValue::Reg);
	source.SetReg(rs);
	dest.SetReg(rd);
	source2.SetReg(rt);

	i = IRInstruction::Build({dest, source, source2}, IRInstrs::OR);

	cur_block->ir.push_back(i);
}

void JIT::EmitNOR(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	// printf("nor %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

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

	// printf("slt %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

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

	// printf("sltu %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt));

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

	// printf("daddu %s, %s, %s (0x%08lx, 0x%08lx)\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rs), EmotionEngine::Reg(rt), EmotionEngine::state.regs[rs].u64[0], EmotionEngine::state.regs[rt].u64[0]);

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

void JIT::EmitDSLL(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int sa = (instr >> 6) & 0x1F;

	// printf("dsll %s, %s, %d\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), sa);

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

void JIT::EmitDSLL32(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int sa = (instr >> 6) & 0x1F;

	// printf("dsll32 %s, %s, %d\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), sa);

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

void JIT::EmitDSRA32(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int sa = (instr >> 6) & 0x1F;

	// printf("dsra32 %s, %s, %d\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), sa);

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
	
	// printf("bltz %s, 0x%08x (%x)\n", EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_, i_);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);

	dest.SetReg(rs);
	imm.SetImm32((int32_t)(int16_t)(instr & 0xffff) << 2);

	i = IRInstruction::Build({dest, imm}, IRInstrs::BranchRegImm);
	i.b_type = IRInstruction::BranchType::LT;

	cur_block->ir.push_back(i);
}

void JIT::EmitBGEZ(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rs = (instr >> 21) & 0x1F;

	int32_t i_ = (int32_t)(int16_t)(instr & 0xffff) << 2;
	
	// printf("bgez %s, 0x%08x (%x)\n", EmotionEngine::Reg(rs), EmotionEngine::state.pc_at + i_, i_);

	IRValue dest = IRValue(IRValue::Reg);
	IRValue imm = IRValue(IRValue::Imm);

	dest.SetReg(rs);
	imm.SetImm32((int32_t)(int16_t)(instr & 0xffff) << 2);

	i = IRInstruction::Build({dest, imm}, IRInstrs::BranchRegImm);
	i.b_type = IRInstruction::BranchType::GE;

	cur_block->ir.push_back(i);
}

void JIT::EmitMFLO1(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;

	// printf("mflo1 %s\n", EmotionEngine::Reg(rd));

	IRValue reg = IRValue(IRValue::Reg);
	reg.SetReg(rd);

	i = IRInstruction::Build({reg}, IRInstrs::MoveFromLo);
	i.is_mmi_divmul = true;

	cur_block->ir.push_back(i);
}

void JIT::EmitMULT1(uint32_t instr, EE_JIT::IRInstruction &i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;
	int rs = (instr >> 21) & 0x1F;

	// printf("mult1 %s, %s, %s\n", EmotionEngine::Reg(rd), EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

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

	// printf("divu1 %s, %s\n", EmotionEngine::Reg(rt), EmotionEngine::Reg(rs));

	IRValue src1 = IRValue(IRValue::Reg);
	IRValue src2 = IRValue(IRValue::Reg);
	
	src1.SetReg(rt);
	src2.SetReg(rs);

	i = IRInstruction::Build({src1, src2}, IRInstrs::Div);
	i.is_unsigned = true;
	i.is_mmi_divmul = true;

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

void JIT::EmitMFC0(uint32_t instr, EE_JIT::IRInstruction& i)
{
	int rd = (instr >> 11) & 0x1F;
	int rt = (instr >> 16) & 0x1F;

	// printf("mfc0 %s, r%d (0x%08x)\n", EmotionEngine::Reg(rt), rd, EmotionEngine::state.cop0_regs[rd]);

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

	// printf("mtc0 %s, r%d\n", EmotionEngine::Reg(rt), rd);

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
		// printf("nop\n");
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
		case 0x0D:
			EmitBreak(instr, current_instruction);
			break;
		case 0x0F:
			// printf("sync\n");
			break;
		case 0x10:
			EmitMFHI(instr, current_instruction);
			break;
		case 0x12:
			EmitMFLO(instr, current_instruction);
			break;
		case 0x14:
			EmitDSLLV(instr, current_instruction);
			break;
		case 0x17:
			EmitDSRAV(instr, current_instruction);
			break;
		case 0x18:
			EmitMULT(instr, current_instruction);
			break;
		case 0x1A:
			EmitDIV(instr, current_instruction);
			break;
		case 0x1B:
			EmitDIVU(instr, current_instruction);
			break;
		case 0x21:
			EmitADDU(instr, current_instruction);
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
		case 0x27:
			EmitNOR(instr, current_instruction);
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
		case 0x38:
			EmitDSLL(instr, current_instruction);
			break;
		case 0x3C:
			EmitDSLL32(instr, current_instruction);
			break;
		case 0x3F:
			EmitDSRA32(instr, current_instruction);
			break;
		default:
			// printf("[emu/CPU]: Cannot emit unknown special instruction 0x%02x\n", opcode);
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
		default:
			// printf("Unknown regimm opcode 0x%02x\n", opcode);
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
		default:
			printf("[emu/EE]: Ignoring COP1 command 0x%08x\n", instr);
			break;
		}
		break;
	}
	case 0x12:
	{
		opcode = (instr >> 21) & 0x1F;
		switch (opcode)
		{
		case 0x02:
		case 0x06:
			printf("[WARNING]: Ignoring CFC2/CTC2\n");
			break;
		case 0x10 ... 0x3F:
			opcode = instr & 0x3F;
			switch (opcode)
			{
			case 0x3c ... 0x3f:
			{
				uint8_t fhi = (instr >> 6) & 0x1F;
				uint8_t flo = instr & 0b11;
				opcode = flo | (fhi * 4);

				switch (opcode)
				{
				default:
					printf("[emu/CPU]: Cannot emit unknown cop2 special2 instruction 0x%02x (0x%08x)\n", opcode, instr);
					break;
				}
				break;
			}
			default:
				printf("[emu/CPU]: Cannot emit unknown cop2 special1 instruction 0x%02x\n", opcode);
				break;
			}
			break;
		default:
			printf("[emu/CPU]: Cannot emit unknown cop2 instruction 0x%02x\n", opcode);
			break;
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
		case 0x12:
			EmitMFLO1(instr, current_instruction);
			break;
		case 0x18:
			EmitMULT1(instr, current_instruction);
			break;
		case 0x1B:
			EmitDIVU1(instr, current_instruction);
			break;
		case 0x29:
		{
			opcode = (instr >> 5) & 0x1F;

			switch (opcode)
			{
			case 0x05:
				EmitPOR(instr, current_instruction);
				break;
			default:
				printf("[emu/CPU]: Cannot emit unknown mmi3 instruction 0x%02x\n", opcode);
				delete cur_block;
				exit(1);
			}
			break;
		}
		default:
			printf("[emu/CPU]: Cannot emit unknown mmi instruction 0x%02x\n", opcode);
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
	case 0x37:
		EmitLD(instr, current_instruction);
		break;
	case 0x39:
		EmitSWC1(instr, current_instruction);
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
	cur_block = new Block;
	cur_block->guest_addr = guest_start;
	cur_block->entry = (uint8_t*)emit->GetFreeBase();
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
	if (!blockCache[start])
	{
		// printf("ERROR: Block doesn't exist at 0x%08x\n", start);
		exit(1);
	}

	return blockCache[start];
}

bool JIT::DoesBlockExist(uint32_t addr)
{
	return blockCache.find(addr) != blockCache.end();
}

void JIT::CheckCacheFull()
{
	if (blockCache.size() >= 0x2000)
	{
		emit->ResetFreeBase();
		for (auto b : blockCache)
			delete b.second;
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

}

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
	default:
		return false;
	}

	return false;
}

EE_JIT::JIT::EntryFunc EmitDone(int cycles_taken)
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
	printf("hi\t->\t0x%08x\n", state.hi);
	printf("lo\t->\t0x%08x\n", state.lo);
	printf("hi1\t->\t0x%08x\n", state.hi1);
	printf("lo1\t->\t0x%08x\n", state.lo1);
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
}
