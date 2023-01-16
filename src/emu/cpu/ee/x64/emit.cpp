// (c) Copyright 2022 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include "emit.h"
#include <sys/mman.h>
#include <emu/cpu/ee/EmotionEngine.h>
#include <emu/memory/Bus.h>
#include <emu/sched/scheduler.h>
#include <fstream>
#include <cassert>

EE_JIT::Emitter* EE_JIT::emit;

void EE_JIT::Emitter::EmitMov(IRInstruction i)
{
	if (i.args[1].IsImm()) // Lui
	{
		Xbyak::Reg64 reg_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 reg = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg32 imm_ext_ptr = Xbyak::Reg32(reg_alloc->AllocHostRegister());
		auto offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[0].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u32)));
		cg->lea(reg_ptr, cg->ptr[cg->rbp + offset]);
		cg->mov(imm_ext_ptr, i.args[1].GetImm());
		cg->movsxd(reg, imm_ext_ptr);
		cg->mov(cg->qword[reg_ptr], reg);
	}
	else if (i.args[1].IsCop0()) // Mfc0
	{
		Xbyak::Reg64 ee_reg_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 cop_reg_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg32 cop_reg_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[0].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u32)));
		auto src_offset = ((offsetof(EmotionEngine::ProcessorState, cop0_regs)) + (i.args[1].GetReg() * sizeof(uint32_t)));
		cg->lea(cop_reg_ptr, cg->ptr[cg->rbp + src_offset]);
		cg->mov(cop_reg_value, cg->dword[cop_reg_ptr]);
		cg->lea(ee_reg_ptr, cg->ptr[cg->rbp + dest_offset]);
		cg->mov(cg->dword[ee_reg_ptr], cop_reg_value);
	}
	else if (i.args[1].IsReg()) // Mtc0
	{
		if (i.args[0].IsCop0())
		{
			Xbyak::Reg64 ee_reg_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
			Xbyak::Reg64 cop_reg_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
			Xbyak::Reg32 ee_reg_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
			auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[1].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u32)));
			auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, cop0_regs)) + (i.args[0].GetReg() * sizeof(uint32_t)));
			cg->lea(cop_reg_ptr, cg->ptr[cg->rbp + dest_offset]);
			cg->lea(ee_reg_ptr, cg->ptr[cg->rbp + src_offset]);
			cg->mov(ee_reg_value, cg->dword[ee_reg_ptr]);
			cg->mov(cg->dword[cop_reg_ptr], ee_reg_value);
		}
		else
		{
			printf("[emu/JIT]: Unknown MOV format with 2nd argument reg\n");
			exit(1);
		}
	}
	else
	{
		printf("[emu/JIT]: Unknown MOV format\n");
		exit(1);
	}
}

void EE_JIT::Emitter::EmitMovCond(IRInstruction i)
{
	Xbyak::Reg64 ee_rt = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg64 ee_src = Xbyak::Reg64(reg_alloc->AllocHostRegister());

	auto rt_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[1].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u32)));
	auto dst_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[0].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u32)));
	auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[2].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u32)));

	cg->mov(ee_rt, cg->qword[cg->rbp + rt_offset]);
	cg->cmp(ee_rt, 0);

	Xbyak::Label end;

	if (i.mov_cond == IRInstruction::MovCond::N)
	{
		cg->jne(end);
	}
	else
	{
		cg->je(end);
	}

	cg->mov(ee_src, cg->qword[cg->rbp + src_offset]);
	cg->mov(cg->qword[cg->rbp + dst_offset], ee_src);
	
	cg->L(end);
}

void EE_JIT::Emitter::EmitSLTI(IRInstruction i)
{
	if (!i.args[2].IsImm())
	{
		Xbyak::Reg32 ee_src1_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
		Xbyak::Reg32 ee_src2_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 ee_64_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[0].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u32)));
		auto src1_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[1].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u32)));
		auto src2_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[2].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u32)));

		cg->mov(ee_src1_value, cg->dword[cg->rbp + src1_offset]);
		cg->mov(ee_src2_value, cg->dword[cg->rbp + src2_offset]);
		cg->cmp(ee_src1_value, ee_src2_value);
		Xbyak::Label not_lt;
		Xbyak::Label done;
		if (i.is_unsigned)
			cg->jae(not_lt);
		else
			cg->jge(not_lt);
		cg->mov(ee_64_value, 1);
		cg->mov(cg->qword[cg->rbp + dest_offset], ee_64_value);
		cg->jmp(done);
		cg->L(not_lt);
		cg->mov(ee_64_value, 0);
		cg->mov(cg->qword[cg->rbp + dest_offset], ee_64_value);
		cg->L(done);
	}
	else
	{
		Xbyak::Reg64 ee_dest_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 ee_src_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg32 ee_src_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 ee_64_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[0].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u32)));
		auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[1].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u32)));
				
		cg->lea(ee_src_ptr, cg->ptr[cg->rbp + src_offset]);
		cg->mov(ee_src_value, cg->dword[ee_src_ptr]);
		cg->cmp(ee_src_value, i.args[2].GetImm());
		Xbyak::Label not_lt;
		Xbyak::Label done;
		cg->lea(ee_dest_ptr, cg->ptr[cg->rbp + dest_offset]);
		if (i.is_unsigned)
			cg->jae(not_lt);
		else
			cg->jge(not_lt);
		cg->mov(ee_64_value, 1);
		cg->mov(cg->qword[cg->rbp + dest_offset], ee_64_value);
		cg->jmp(done);
		cg->L(not_lt);
		cg->mov(ee_64_value, 0);
		cg->mov(cg->qword[cg->rbp + dest_offset], ee_64_value);
		cg->L(done);
	}
}

void EE_JIT::Emitter::EmitBC(IRInstruction i)
{
	if (!i.args[2].IsImm())
	{
		printf("[emu/JIT]: Argument 2 to branch is not immediate\n");
		exit(1);
	}

	Xbyak::Reg64 ee_reg1_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg64 ee_reg2_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg64 ee_reg1_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg64 ee_reg2_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg32 next_pc = Xbyak::Reg32(reg_alloc->AllocHostRegister());
	auto reg1_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[0].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u64)));
	auto reg2_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[1].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u64)));
	
	cg->lea(ee_reg1_ptr, cg->ptr[cg->rbp + reg1_offset]);
	cg->mov(ee_reg1_value, cg->dword[ee_reg1_ptr]);
	cg->lea(ee_reg2_ptr, cg->ptr[cg->rbp + reg2_offset]);
	cg->mov(ee_reg2_value, cg->dword[ee_reg2_ptr]);
	cg->cmp(ee_reg1_value, ee_reg2_value);
	Xbyak::Label conditional_failed;
	Xbyak::Label end;

	if (i.b_type == IRInstruction::NE)
	{
		cg->je(conditional_failed);
		cg->mov(next_pc, cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, pc)]);
		cg->add(next_pc, i.args[2].GetImm());
		cg->mov(cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)], next_pc);
		cg->jmp(end);
	}
	else if (i.b_type == IRInstruction::EQ)
	{
		cg->jne(conditional_failed);
		cg->mov(next_pc, cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, pc)]);
		cg->add(next_pc, i.args[2].GetImm());
		cg->mov(cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)], next_pc);
		cg->jmp(end);
	}
	else
	{
		printf("[emu/JIT]: Unknown branch conditional type %d\n", i.b_type);
		exit(1);
	}

	cg->L(conditional_failed);

	if (i.is_likely)
	{
		// cg->lea(next_pc, cg->ptr[cg->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)]);
		// cg->add(cg->dword[next_pc], 4);
		reg_alloc->Reset();
		EmitIncPC(i);
		cg->ret();
	}

	cg->L(end);
}

void EE_JIT::Emitter::EmitIncPC(IRInstruction)
{
	Xbyak::Reg64 next_pc_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg64 pc_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg32 next_pc = Xbyak::Reg32(reg_alloc->AllocHostRegister());
	cg->lea(next_pc_ptr, cg->ptr[cg->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)]);
	cg->mov(next_pc, cg->dword[next_pc_ptr]);
	cg->lea(pc_ptr, cg->ptr[cg->rbp + offsetof(EmotionEngine::ProcessorState, pc)]);
	cg->mov(cg->dword[pc_ptr], next_pc);
	cg->add(next_pc, 4);
	cg->mov(cg->dword[next_pc_ptr], next_pc);
}

void EE_JIT::Emitter::EmitOR(IRInstruction i)
{
	if (i.args[2].IsImm())
	{
		Xbyak::Reg64 src_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 dest_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 src_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());

		auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));


		cg->lea(src_ptr, cg->ptr[cg->rbp + src_offset]);
		cg->mov(src_value, cg->dword[src_ptr]);
		cg->or_(src_value, i.args[2].GetImm());
		cg->lea(dest_ptr, cg->ptr[cg->rbp + dest_offset]);
		cg->mov(cg->dword[dest_ptr], src_value);
	}
	else if (i.args[2].IsReg())
	{
		Xbyak::Reg64 src1 = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 src2 = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		auto src1_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto src2_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));

		cg->mov(src1, cg->dword[cg->rbp + src1_offset]);
		cg->mov(src2, cg->dword[cg->rbp + src2_offset]);
		cg->or_(src1, src2);
		cg->mov(cg->dword[cg->rbp + dest_offset], src1);
	}
	else
	{
		printf("[emu/JIT]: Unknown OR argument\n");
		exit(1);
	}
}

void EE_JIT::Emitter::EmitNOR(IRInstruction i)
{
	if (i.args[2].IsImm())
	{
		Xbyak::Reg64 src_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 dest_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 src_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());

		auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));


		cg->lea(src_ptr, cg->ptr[cg->rbp + src_offset]);
		cg->mov(src_value, cg->dword[src_ptr]);
		cg->or_(src_value, i.args[2].GetImm());
		cg->not_(src_value);
		cg->lea(dest_ptr, cg->ptr[cg->rbp + dest_offset]);
		cg->mov(cg->dword[dest_ptr], src_value);
	}
	else if (i.args[2].IsReg())
	{
		Xbyak::Reg64 src1 = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 src2 = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		auto src1_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto src2_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));

		cg->mov(src1, cg->dword[cg->rbp + src1_offset]);
		cg->mov(src2, cg->dword[cg->rbp + src2_offset]);
		cg->or_(src1, src2);
		cg->not_(src1);
		cg->mov(cg->dword[cg->rbp + dest_offset], src1);
	}
	else
	{
		printf("[emu/JIT]: Unknown OR argument\n");
		exit(1);
	}
}

void EE_JIT::Emitter::EmitXOR(IRInstruction i)
{
	if (i.args[2].IsImm())
	{
		Xbyak::Reg64 src_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 dest_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 src_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());

		auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));


		cg->lea(src_ptr, cg->ptr[cg->rbp + src_offset]);
		cg->mov(src_value, cg->dword[src_ptr]);
		cg->xor_(src_value, i.args[2].GetImm());
		cg->lea(dest_ptr, cg->ptr[cg->rbp + dest_offset]);
		cg->mov(cg->dword[dest_ptr], src_value);
	}
	else if (i.args[2].IsReg())
	{
		Xbyak::Reg64 src1 = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 src2 = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		auto src1_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto src2_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));

		cg->mov(src1, cg->dword[cg->rbp + src1_offset]);
		cg->mov(src2, cg->dword[cg->rbp + src2_offset]);
		cg->xor_(src1, src2);
		cg->mov(cg->dword[cg->rbp + dest_offset], src1);
	}
	else
	{
		printf("[emu/JIT]: Unknown XOR argument\n");
		exit(1);
	}
}

void EE_JIT::Emitter::EmitAND(IRInstruction i)
{
	if (i.args[2].IsImm())
	{
		Xbyak::Reg64 src_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 dest_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 src_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());

		auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));


		cg->lea(src_ptr, cg->ptr[cg->rbp + src_offset]);
		cg->mov(src_value, cg->dword[src_ptr]);
		cg->and_(src_value, i.args[2].GetImm());
		cg->lea(dest_ptr, cg->ptr[cg->rbp + dest_offset]);
		cg->mov(cg->dword[dest_ptr], src_value);
	}
	else if (i.args[2].IsReg())
	{
		Xbyak::Reg64 src1_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 src2_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 dest_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 src1_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 src2_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());

		auto src1_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto src2_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));


		cg->lea(src1_ptr, cg->ptr[cg->rbp + src1_offset]);
		cg->mov(src1_value, cg->dword[src1_ptr]);
		cg->lea(src2_ptr, cg->ptr[cg->rbp + src2_offset]);
		cg->mov(src2_value, cg->dword[src2_ptr]);
		cg->and_(src1_value, src2_value);
		cg->lea(dest_ptr, cg->ptr[cg->rbp + dest_offset]);
		cg->mov(cg->dword[dest_ptr], src1_value);
	}
	else
	{
		printf("[emu/JIT]: Unknown AND argument\n");
		exit(1);
	}
}

void EE_JIT::Emitter::EmitJA(IRInstruction i)
{

	if (i.should_link)
	{
		Xbyak::Reg64 link_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 link_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 next_pc_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg32 next_pc_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());

		auto offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		
		cg->lea(next_pc_ptr, cg->ptr[cg->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)]);
		cg->mov(next_pc_value, cg->dword[next_pc_ptr]);
		cg->lea(link_ptr, cg->ptr[cg->rbp + offset]);
		cg->mov(cg->dword[link_ptr], next_pc_value);
		reg_alloc->Reset();
	}
	if (i.args[0].IsReg())
	{
		Xbyak::Reg64 next_pc_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 reg_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg32 reg_val = Xbyak::Reg32(reg_alloc->AllocHostRegister());

		auto offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));

		cg->lea(reg_ptr, cg->ptr[cg->rbp + offset]);
		cg->lea(next_pc_ptr, cg->ptr[cg->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)]);
		cg->mov(reg_val, cg->dword[reg_ptr]);
		cg->mov(cg->dword[next_pc_ptr], reg_val);
	}
	else
	{
		printf("[emu/JIT]: Unknown JumpAlways argument\n");
		exit(1);
	}
}

void EE_JIT::Emitter::EmitJumpImm(IRInstruction i)
{
	if (i.should_link)
	{
		Xbyak::Reg32 next_pc_val = Xbyak::Reg32(reg_alloc->AllocHostRegister());
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (31 * sizeof(uint128_t)) + offsetof(uint128_t, u32)));


		cg->mov(next_pc_val, cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)]);
		cg->mov(cg->dword[cg->rbp + dest_offset], next_pc_val);
		reg_alloc->Reset();
	}
	
	Xbyak::Reg32 cur_pc_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
	
	cg->mov(cur_pc_value, cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, pc)]);
	cg->and_(cur_pc_value, 0xf0000000);
	cg->or_(cur_pc_value, i.args[0].GetImm());
	cg->mov(cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)], cur_pc_value);
}

void EE_JIT::Emitter::EmitAdd(IRInstruction i)
{
	if (i.args.size() != 3)
	{
		printf("[emu/IRTranslator]: Invalid argument count %ld\n", i.args.size());
		exit(1);
	}

	if (i.args[2].IsImm())
	{
		if (i.size == IRInstruction::Size32)
		{
			Xbyak::Reg32 ee_src_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
			Xbyak::Reg64 ee_src_ex_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
			auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
			auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));

			cg->mov(ee_src_value, cg->dword[cg->rbp + src_offset]);
			cg->add(ee_src_value, i.args[2].GetImm());
			cg->movsxd(ee_src_ex_value, ee_src_value);
			cg->mov(cg->qword[cg->rbp + dest_offset], ee_src_ex_value);
		}
		else
		{
			Xbyak::Reg64 ee_src_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
			auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
			auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));

			cg->mov(ee_src_value, cg->qword[cg->rbp + src_offset]);
			cg->add(ee_src_value, i.args[2].GetImm());
			cg->mov(cg->qword[cg->rbp + dest_offset], ee_src_value);
		}
	}
	else if (i.args[2].IsReg())
	{
		if (i.size == IRInstruction::Size64)
		{
			Xbyak::Reg64 ee_src1_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
			Xbyak::Reg64 ee_src2_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
			
			auto src2_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
			auto src1_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
			auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));

			cg->mov(ee_src1_value, cg->qword[cg->rbp + src1_offset]);
			cg->mov(ee_src2_value, cg->qword[cg->rbp + src2_offset]);
			cg->add(ee_src1_value, ee_src2_value);
			cg->mov(cg->qword[cg->rbp + dest_offset], ee_src1_value);
		}
		else
		{
			Xbyak::Reg32 ee_src1_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
			Xbyak::Reg32 ee_src2_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
			Xbyak::Reg64 ee_src2_ex_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
			

			auto src1_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
			auto src2_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
			auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));

			cg->mov(ee_src1_value, cg->dword[cg->rbp + src1_offset]);
			cg->mov(ee_src2_value, cg->dword[cg->rbp + src2_offset]);
			cg->add(ee_src1_value, ee_src2_value);
			cg->movsxd(ee_src2_ex_value, ee_src1_value);
			cg->mov(cg->dword[cg->rbp + dest_offset], ee_src2_ex_value);
		}
	}
	else
	{
		printf("Unknown argument type for add\n");
		exit(1);
	}
}

void EE_JIT::Emitter::EmitSub(IRInstruction i)
{
	if (i.args.size() != 3)
	{
		printf("[emu/IRTranslator]: Invalid argument count %ld\n", i.args.size());
		exit(1);
	}

	if (i.args[2].IsImm())
	{
		if (i.size == IRInstruction::Size32)
		{
			Xbyak::Reg32 ee_src_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
			auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
			auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));

			cg->mov(ee_src_value, cg->dword[cg->rbp + src_offset]);
			cg->sub(ee_src_value, i.args[2].GetImm());
			cg->mov(cg->qword[cg->rbp + dest_offset], ee_src_value);
		}
		else
		{
			Xbyak::Reg64 ee_src_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
			auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
			auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));

			cg->mov(ee_src_value, cg->qword[cg->rbp + src_offset]);
			cg->sub(ee_src_value, i.args[2].GetImm());
			cg->mov(cg->qword[cg->rbp + dest_offset], ee_src_value);
		}
	}
	else if (i.args[2].IsReg())
	{
		if (i.size == IRInstruction::Size64)
		{
			Xbyak::Reg64 ee_src1_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
			Xbyak::Reg64 ee_src2_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
			
			auto src1_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
			auto src2_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
			auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));

			cg->mov(ee_src1_value, cg->qword[cg->rbp + src1_offset]);
			cg->mov(ee_src2_value, cg->qword[cg->rbp + src2_offset]);
			cg->sub(ee_src1_value, ee_src2_value);
			cg->mov(cg->qword[cg->rbp + dest_offset], ee_src1_value);
		}
		else
		{
			Xbyak::Reg32 ee_src1_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
			Xbyak::Reg32 ee_src2_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
			
			auto src1_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
			auto src2_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
			auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));

			cg->mov(ee_src1_value, cg->dword[cg->rbp + src1_offset]);
			cg->mov(ee_src2_value, cg->dword[cg->rbp + src2_offset]);
			cg->sub(ee_src1_value, ee_src2_value);
			cg->mov(cg->dword[cg->rbp + dest_offset], ee_src1_value);
		}
	}
	else
	{
		printf("Unknown argument type for sub\n");
		exit(1);
	}
}

void EE_JIT::Emitter::EmitMemoryStore(IRInstruction i)
{
	if (i.args.size() != 3 || !i.args[1].IsImm() || (!i.args[0].IsReg() && !i.args[0].IsFloat()) || !i.args[2].IsReg())
	{
		printf("[emu/IRTranslator]: Invalid argument count/type %ld for MemoryStore\n", i.args.size());
		exit(1);
	}

	reg_alloc->MarkRegUsed(RegisterAllocator::RSI);
	reg_alloc->MarkRegUsed(RegisterAllocator::RDI);
	reg_alloc->MarkRegUsed(RegisterAllocator::RDX);

	Xbyak::Reg32 ee_base_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
	auto base_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
	auto val_offset = i.args[0].IsFloat() ? (offsetof(EmotionEngine::ProcessorState, fprs) + (i.args[0].GetReg() * sizeof(float))) : ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
	Xbyak::Reg64 func_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());

	cg->mov(ee_base_value, cg->qword[cg->rbp + base_offset]);
	cg->add(ee_base_value, i.args[1].GetImm());
	cg->mov(cg->rdi, ee_base_value);
	cg->mov(cg->rsi, cg->dword[cg->rbp + val_offset]);

	if (i.access_size == IRInstruction::U8)
	{
		cg->mov(func_ptr, reinterpret_cast<uint64_t>(&Bus::Write8));
	}
	else if (i.access_size == IRInstruction::U16)
	{
		cg->mov(func_ptr, reinterpret_cast<uint64_t>(&Bus::Write16));
	}
	else if (i.access_size == IRInstruction::U32)
	{
		cg->mov(func_ptr, reinterpret_cast<uint64_t>(&Bus::Write32));
	}
	else if (i.access_size == IRInstruction::U64)
	{
		cg->mov(func_ptr, reinterpret_cast<uint64_t>(&Bus::Write64));
	}
	else if (i.access_size == IRInstruction::U128)
	{
		cg->mov(cg->rdx, cg->dword[cg->rbp + val_offset + sizeof(uint64_t)]);
		cg->mov(func_ptr, reinterpret_cast<uint64_t>(&Bus::Write128));
	}
	else
	{
		printf("Unknown access size %d\n", i.access_size);
		exit(1);
	}

	cg->call(func_ptr);
}

void EE_JIT::Emitter::EmitMemoryLoad(IRInstruction i)
{
	reg_alloc->MarkRegUsed(RegisterAllocator::RDI);
	reg_alloc->MarkRegUsed(RegisterAllocator::RAX);
	reg_alloc->MarkRegUsed(RegisterAllocator::RDX);

	Xbyak::Reg32 ee_base_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
	auto base_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
	auto val_offset = i.args[0].IsFloat() ? (offsetof(EmotionEngine::ProcessorState, fprs) + (i.args[0].GetReg() * sizeof(float))) : ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
	Xbyak::Reg64 func_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());

	cg->mov(ee_base_value, cg->qword[cg->rbp + base_offset]);
	cg->add(ee_base_value, i.args[1].GetImm());
	cg->mov(cg->rdi, ee_base_value);

	if (i.access_size == IRInstruction::U8)
	{
		cg->mov(func_ptr, reinterpret_cast<uint64_t>(&Bus::Read8));
	}
	else if (i.access_size == IRInstruction::U16)
	{
		cg->mov(func_ptr, reinterpret_cast<uint64_t>(&Bus::Read16));
	}
	else if (i.access_size == IRInstruction::U32)
	{
		cg->mov(func_ptr, reinterpret_cast<uint64_t>(&Bus::Read32));
	}
	else if (i.access_size == IRInstruction::U64)
	{
		cg->mov(func_ptr, reinterpret_cast<uint64_t>(&Bus::Read64));
	}
	else if (i.access_size == IRInstruction::U128)
	{
		cg->mov(func_ptr, reinterpret_cast<uint64_t>(&Bus::Read128));
	}
	else
	{
		printf("Unknown access size %d\n", i.access_size);
		exit(1);
	}

	cg->call(func_ptr);

	if (!i.is_unsigned)
	{
		if (i.access_size == IRInstruction::U8)
		{
			cg->movsx(Xbyak::Reg64(RegisterAllocator::RAX), cg->al);
		}
		else if (i.access_size == IRInstruction::U16)
		{
			cg->movsx(Xbyak::Reg64(RegisterAllocator::RAX), cg->ax);
		}
		else if (i.access_size == IRInstruction::U32)
		{
			cg->movsxd(Xbyak::Reg64(RegisterAllocator::RAX), cg->eax);
		}
	}
	
	cg->mov(cg->dword[cg->rbp + val_offset], cg->rax);
	if (i.access_size == IRInstruction::U128)
		cg->mov(cg->dword[cg->rbp + val_offset + sizeof(uint64_t)], cg->rdx);
}

void EE_JIT::Emitter::EmitShift(IRInstruction i)
{
	if (i.args[2].IsImm())
	{
		Xbyak::Reg32 ee_src_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 ee_dest_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));

		cg->mov(ee_src_value, cg->dword[cg->rbp + src_offset]);
		
		if (i.direction == IRInstruction::Left)
		{
			cg->shl(ee_src_value, i.args[2].GetImm());
		}
		else
		{
			if (i.is_logical)
			{
				cg->shr(ee_src_value, i.args[2].GetImm());
			}
			else
			{
				cg->sar(ee_src_value, i.args[2].GetImm());
			}
		}
		cg->movsxd(ee_dest_value, ee_src_value);
		cg->mov(cg->dword[cg->rbp + dest_offset], ee_dest_value);
	}
	else if (i.args[2].IsReg())
	{
		reg_alloc->MarkRegUsed(RegisterAllocator::RCX);
		Xbyak::Reg32 ee_src_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
		auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto shift_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));

		cg->mov(ee_src_value, cg->dword[cg->rbp + src_offset]);
		cg->mov(cg->rcx, cg->dword[cg->rbp + shift_offset]);
		cg->and_(cg->rcx, 0x1F);
		
		if (i.direction == IRInstruction::Left)
		{
			cg->shl(ee_src_value, cg->cl);
		}
		else
		{
			if (i.is_logical)
			{
				cg->shr(ee_src_value, cg->cl);
			}
			else
			{
				cg->sar(ee_src_value, cg->cl);
			}
		}
		cg->mov(cg->dword[cg->rbp + dest_offset], ee_src_value);
	}
	else
	{
		printf("[emu/JIT]: Unknown SHIFT argument\n");
		exit(1);
	}
}

void EE_JIT::Emitter::EmitShift64(IRInstruction i)
{
	if (i.args[2].IsImm())
	{
		Xbyak::Reg64 ee_src_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));

		cg->mov(ee_src_value, cg->dword[cg->rbp + src_offset]);
		
		if (i.direction == IRInstruction::Left)
		{
			cg->shl(ee_src_value, i.args[2].GetImm());
		}
		else
		{
			if (i.is_logical)
			{
				cg->shr(ee_src_value, i.args[2].GetImm());
			}
			else
			{
				cg->sar(ee_src_value, i.args[2].GetImm());
			}
		}
		cg->mov(cg->dword[cg->rbp + dest_offset], ee_src_value);
	}
	else if (i.args[2].IsReg())
	{
		reg_alloc->MarkRegUsed(RegisterAllocator::RCX);
		Xbyak::Reg64 ee_src_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto shift_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));

		cg->mov(ee_src_value, cg->dword[cg->rbp + src_offset]);
		cg->mov(cg->rcx, cg->dword[cg->rbp + shift_offset]);
		cg->and_(cg->rcx, 0x1F);
		
		if (i.direction == IRInstruction::Left)
		{
			cg->shl(ee_src_value, cg->cl);
		}
		else
		{
			if (i.is_logical)
			{
				cg->shr(ee_src_value, cg->cl);
			}
			else
			{
				cg->sar(ee_src_value, cg->cl);
			}
		}
		cg->mov(cg->dword[cg->rbp + dest_offset], ee_src_value);
	}
	else
	{
		printf("[emu/JIT]: Unknown SHIFT64 argument\n");
		exit(1);
	}
}

void EE_JIT::Emitter::EmitMULT(IRInstruction i)
{
	if (!i.is_unsigned)
	{
		Xbyak::Reg64 src1 = Xbyak::Reg64(RegisterAllocator::RAX);
		reg_alloc->MarkRegUsed(RegisterAllocator::RAX);
		reg_alloc->MarkRegUsed(RegisterAllocator::RDX);
		Xbyak::Reg64 src2 = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 dest = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		auto src1_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
		auto src2_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
		auto hi_offset = ((offsetof(EmotionEngine::ProcessorState, hi)));
		auto lo_offset = ((offsetof(EmotionEngine::ProcessorState, lo)));

		cg->mov(src1, cg->dword[cg->rbp + src1_offset]);
		cg->mov(src2, cg->dword[cg->rbp + src2_offset]);
		cg->mul(src2);

		cg->mov(cg->dword[cg->rbp + hi_offset], Xbyak::Reg32(RegisterAllocator::RDX));
		cg->mov(cg->dword[cg->rbp + lo_offset], Xbyak::Reg32(RegisterAllocator::RAX));
		cg->mov(cg->dword[cg->rbp + dest_offset], Xbyak::Reg32(RegisterAllocator::RAX));
	}
	else
	{
		printf("[emu/JIT]: Unhandled unsigned multiply!\n");
		exit(1);
	}
}

void EE_JIT::Emitter::EmitDIV(IRInstruction i)
{
	if (i.is_unsigned)
	{
		reg_alloc->MarkRegUsed(RegisterAllocator::RAX);
		reg_alloc->MarkRegUsed(RegisterAllocator::RDX);
		Xbyak::Reg32 src1 = Xbyak::Reg32(reg_alloc->AllocHostRegister());
		Xbyak::Reg32 src2 = Xbyak::Reg32(RegisterAllocator::RAX);
		auto src1_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto src2_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto hi_offset = i.is_mmi_divmul ? ((offsetof(EmotionEngine::ProcessorState, hi1))) : ((offsetof(EmotionEngine::ProcessorState, hi)));
		auto lo_offset = i.is_mmi_divmul ? ((offsetof(EmotionEngine::ProcessorState, lo1))) : ((offsetof(EmotionEngine::ProcessorState, lo)));
		
		cg->mov(Xbyak::Reg64(RegisterAllocator::RDX), 0);
		cg->mov(src1, cg->dword[cg->rbp + src1_offset]);
		cg->mov(src2, cg->dword[cg->rbp + src2_offset]);
		cg->cmp(src1, 0);
		Xbyak::Label do_divide;
		Xbyak::Label end;

		cg->jne(do_divide);

		cg->mov(cg->dword[cg->rbp + hi_offset], src2);
		cg->mov(cg->dword[cg->rbp + lo_offset], 0xffffffff);

		cg->jmp(end);

		cg->L(do_divide);
		
		cg->div(src1);

		cg->mov(cg->dword[cg->rbp + lo_offset], Xbyak::Reg32(RegisterAllocator::RAX));
		cg->mov(cg->dword[cg->rbp + hi_offset], Xbyak::Reg32(RegisterAllocator::RDX));

		cg->L(end);
	}
	else
	{
		reg_alloc->MarkRegUsed(RegisterAllocator::RAX);
		reg_alloc->MarkRegUsed(RegisterAllocator::RDX);
		Xbyak::Reg32 src1 = Xbyak::Reg32(reg_alloc->AllocHostRegister());
		Xbyak::Reg32 src2 = Xbyak::Reg32(RegisterAllocator::RAX);
		auto src1_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto src2_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto hi_offset = i.is_mmi_divmul ? ((offsetof(EmotionEngine::ProcessorState, hi1))) : ((offsetof(EmotionEngine::ProcessorState, hi)));
		auto lo_offset = i.is_mmi_divmul ? ((offsetof(EmotionEngine::ProcessorState, lo1))) : ((offsetof(EmotionEngine::ProcessorState, lo)));
		
		cg->mov(Xbyak::Reg64(RegisterAllocator::RDX), 0);
		cg->mov(src1, cg->dword[cg->rbp + src1_offset]);
		cg->mov(src2, cg->dword[cg->rbp + src2_offset]);
		cg->cmp(src1, 0);
		Xbyak::Label do_divide;
		Xbyak::Label check_sign;
		Xbyak::Label is_ltz;
		Xbyak::Label end;

		cg->jne(check_sign);

		cg->mov(cg->dword[cg->rbp + hi_offset], src2);

		cg->cmp(src2, 0);
		cg->jl(is_ltz);

		cg->mov(cg->dword[cg->rbp + lo_offset], 0xffffffff);

		cg->jmp(end);

		cg->L(is_ltz);
		
		cg->mov(cg->dword[cg->rbp + lo_offset], 1);

		cg->jmp(end);

		cg->L(check_sign);

		cg->cmp(src1, 0x80000000);
		cg->jne(do_divide);
		cg->cmp(src2, -1);
		cg->jne(do_divide);

		cg->mov(cg->dword[cg->rbp + hi_offset], 0);
		cg->mov(cg->dword[cg->rbp + lo_offset], 0x80000000);

		cg->jmp(end);

		cg->L(do_divide);
		
		cg->div(src1);

		cg->mov(cg->dword[cg->rbp + lo_offset], Xbyak::Reg32(RegisterAllocator::RAX));
		cg->mov(cg->dword[cg->rbp + hi_offset], Xbyak::Reg32(RegisterAllocator::RDX));

		cg->L(end);
	}
}

void EE_JIT::Emitter::EmitMoveFromLo(IRInstruction i)
{
	Xbyak::Reg64 lo_val = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
	auto lo_offset = i.is_mmi_divmul ? ((offsetof(EmotionEngine::ProcessorState, lo1))) : ((offsetof(EmotionEngine::ProcessorState, lo)));
	cg->mov(lo_val, cg->dword[cg->rbp + lo_offset]);
	cg->mov(cg->qword[cg->rbp + dest_offset], lo_val);
}

void EE_JIT::Emitter::EmitMoveFromHi(IRInstruction i)
{
	Xbyak::Reg32 hi_val = Xbyak::Reg32(reg_alloc->AllocHostRegister());
	auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
	auto hi_offset = ((offsetof(EmotionEngine::ProcessorState, hi)));

	cg->mov(hi_val, cg->dword[cg->rbp + hi_offset]);
	cg->mov(cg->dword[cg->rbp + dest_offset], hi_val);
}

void EE_JIT::Emitter::EmitBranchRegImm(IRInstruction i)
{
	auto ee_reg_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	auto next_pc = Xbyak::Reg32(reg_alloc->AllocHostRegister());
	auto reg_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
	
	cg->mov(ee_reg_value, cg->qword[cg->rbp + reg_offset]);
	cg->cmp(ee_reg_value, 0);

	Xbyak::Label cond_failed;

	if (i.b_type == IRInstruction::BranchType::GE)
	{
		cg->jl(cond_failed);
	}
	else if (i.b_type == IRInstruction::BranchType::LE)
	{
		cg->jg(cond_failed);
	}
	else if (i.b_type == IRInstruction::BranchType::GT)
	{
		cg->jle(cond_failed);
	}
	else if (i.b_type == IRInstruction::BranchType::LT)
	{
		cg->jge(cond_failed);
	}
	else
	{
		printf("[emu/JIT]: Unknown regimm branch type %d\n", i.b_type);
		exit(1);
	}

	cg->mov(next_pc, cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, pc)]);
	cg->add(next_pc, i.args[1].GetImm());
	cg->mov(cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)], next_pc);

	cg->L(cond_failed);
}

void EE_JIT::Emitter::EmitUpdateCopCount(IRInstruction i)
{
	Xbyak::Reg32 cop0_count = Xbyak::Reg32(reg_alloc->AllocHostRegister());
	auto offset = ((offsetof(EmotionEngine::ProcessorState, cop0_regs)) + (9 * sizeof(uint32_t)));

	cg->mov(cop0_count, cg->dword[cg->rbp + offset]);
	cg->add(cop0_count ,i.args[0].GetImm());
	cg->mov(cg->dword[cg->rbp + offset], cop0_count);
}

void EE_JIT::Emitter::EmitPOR(IRInstruction i)
{
	Xbyak::Xmm src1 = Xbyak::Xmm(reg_alloc->AllocHostXMMRegister());
	Xbyak::Xmm src2 = Xbyak::Xmm(reg_alloc->AllocHostXMMRegister());
	auto reg1_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u128)));
	auto reg2_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u128)));
	auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u128)));
	
	cg->movaps(src1, cg->ptr[cg->rbp + reg1_offset]);
	cg->movaps(src2, cg->ptr[cg->rbp + reg2_offset]);

	cg->por(src1, src2);

	cg->movaps(cg->ptr[cg->rbp + dest_offset], src1);
}

void HandleUhOh()
{
	printf("[emu/JIT]: Uh oh, break got executed!\n");
	exit(1);
}

void EE_JIT::Emitter::EmitUhOh(IRInstruction i)
{
	cg->mov(cg->rax, reinterpret_cast<uint64_t>(HandleUhOh));
	cg->call(cg->rax);
}

void EE_JIT::Emitter::EmitSaveHostRegs()
{
	sizeWithoutCurBlock = cg->getSize();
	for (int i = 0; i < 8; i++)
		if (i != 4)
			cg->push(Xbyak::Reg64(i));
		
	cg->mov(cg->rbp, reinterpret_cast<uint64_t>(EmotionEngine::GetState()));
}

void EE_JIT::Emitter::EmitRestoreHostRegs()
{
	for (int i = 7; i >= 0; i--)
		if (i != 4)
			cg->pop(Xbyak::Reg64(i));
	cg->ret();

	free_base += (cg->getSize() - sizeWithoutCurBlock);
}

void EE_JIT::Emitter::EmitIR(IRInstruction i)
{
	reg_alloc->Reset();

	switch (i.instr)
	{
	case MOV:
		EmitMov(i);
		break;
	case NOP:
		cg->nop();
		break;
	case SLTI:
		EmitSLTI(i);
		break;
	case SaveHostRegs:
		EmitSaveHostRegs();
		break;
	case RestoreHostRegs:
		EmitRestoreHostRegs();
		break;
	case BranchConditional:
		EmitBC(i);
		break;
	case IncPC:
		EmitIncPC(i);
		break;
	case OR:
		EmitOR(i);
		break;
	case JumpAlways:
		EmitJA(i);
		break;
	case Add:
		EmitAdd(i);
		break;
	case MemoryStore:
		EmitMemoryStore(i);
		break;
	case JumpImm:
		EmitJumpImm(i);
		break;
	case AND:
		EmitAND(i);
		break;
	case Shift:
		EmitShift(i);
		break;
	case Mult:
		EmitMULT(i);
		break;
	case Div:
		EmitDIV(i);
		break;
	case UhOh:
		EmitUhOh(i);
		break;
	case MemoryLoad:
		EmitMemoryLoad(i);
		break;
	case MoveFromLo:
		EmitMoveFromLo(i);
		break;
	case BranchRegImm:	
		EmitBranchRegImm(i);
		break;
	case MoveFromHi:
		EmitMoveFromHi(i);
		break;
	case Sub:
		EmitSub(i);
		break;
	case MovCond:
		EmitMovCond(i);
		break;
	case Shift64:
		EmitShift64(i);
		break;
	case XOR:
		EmitXOR(i);
		break;
	case UpdateCopCount:
		EmitUpdateCopCount(i);
		break;
	case POR:
		EmitPOR(i);
		break;
	case NOR:
		EmitNOR(i);
		break;
	default:
		printf("[JIT/Emit]: Unknown IR instruction %d\n", i.instr);
		exit(1);
	}
}

EE_JIT::Emitter::Emitter()
{
	base = (uint8_t*)mmap(nullptr, 0xffffffff, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (base == MAP_FAILED)
	{
		printf("[JIT/Emit]: Failed to map base! %s\n", strerror(errno));
		exit(1);
	}

	free_base = base;

	cg = new Xbyak::CodeGenerator(0xffffff, (void*)base);
	reg_alloc = new RegisterAllocator();
	reg_alloc->Reset();

	reg_alloc->MarkRegUsed(RegisterAllocator::RDI);
	reg_alloc->MarkRegUsed(RegisterAllocator::RAX);

	Xbyak::Reg64 func_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg32 next_pc = Xbyak::Reg32(reg_alloc->AllocHostRegister());
	Xbyak::Reg32 pc = Xbyak::Reg32(reg_alloc->AllocHostRegister());
	Xbyak::Reg32 isBranchDelayed = Xbyak::Reg32(reg_alloc->AllocHostRegister());
	
	cg->endbr64();
	EmitSaveHostRegs();

	// RSP + 0: pc
	// RSP + 4: next_pc
	// RSP + 8: isBranch
	// RSP + 16: isBranchDelayed
	// RSP + 20: instrs_emitted
	// RSP + 24 .. RSP + 32: Wasted space for alignment reasons
	cg->sub(cg->rsp, 32);

	cg->mov(cg->dword[cg->rsp + 0], 0);
	cg->mov(cg->dword[cg->rsp + 4], 0);
	cg->mov(cg->dword[cg->rsp + 8], 0);
	cg->mov(cg->dword[cg->rsp + 16], 0);
	cg->mov(cg->dword[cg->rsp + 20], 0);
	cg->mov(cg->dword[cg->rsp + 24], 0);

	Xbyak::Label begin;
	cg->L(begin);

	cg->mov(func_ptr, reinterpret_cast<uint64_t>(EmotionEngine::CheckCacheFull));
	cg->call(func_ptr);
	
	auto next_pc_off = (offsetof(EmotionEngine::ProcessorState, next_pc));
	auto pc_off = (offsetof(EmotionEngine::ProcessorState, pc));

	cg->mov(next_pc, cg->dword[cg->rbp + next_pc_off]);
	cg->mov(pc, cg->dword[cg->rbp + pc_off]);

	cg->mov(cg->dword[cg->rsp], pc);
	cg->mov(cg->dword[cg->rsp + 4], next_pc);

	cg->mov(cg->edi, cg->dword[cg->rsp]);
	cg->mov(func_ptr, reinterpret_cast<uint64_t>(EmotionEngine::DoesBlockExit));
	cg->call(func_ptr);

	cg->test(cg->al, cg->al);

	Xbyak::Label block;
	cg->jne(block, Xbyak::CodeGenerator::T_NEAR);

	cg->mov(func_ptr, reinterpret_cast<uint64_t>(EmotionEngine::EmitPrequel));
	cg->call(func_ptr);

	Xbyak::Label compile_instr;
	cg->L(compile_instr);

	cg->add(cg->dword[cg->rsp + 20], 1);
	cg->mov(cg->edi, cg->dword[cg->rsp]);
	cg->mov(func_ptr, reinterpret_cast<uint64_t>(Bus::Read32));
	cg->call(func_ptr);
	
	cg->mov(next_pc, cg->dword[cg->rsp + 4]);
	cg->mov(cg->dword[cg->rsp], next_pc);
	cg->add(cg->dword[cg->rsp + 4], 4);

	cg->mov(cg->rdi, cg->rax);
	cg->mov(func_ptr, reinterpret_cast<uint64_t>(EmotionEngine::EmitIR));
	cg->call(func_ptr);

	cg->mov(isBranchDelayed, cg->dword[cg->rsp + 16]);
	cg->mov(cg->dword[cg->rsp + 8], isBranchDelayed);
	
	
	cg->mov(cg->edi, cg->dword[cg->rsp]);
	cg->sub(cg->edi, 4);
	cg->mov(func_ptr, reinterpret_cast<uint64_t>(Bus::Read32));
	cg->call(func_ptr);
	cg->mov(cg->edi, cg->eax);
	cg->mov(cg->eax, 0);
	cg->mov(func_ptr, reinterpret_cast<uint64_t>(EmotionEngine::IsBranch));
	cg->call(func_ptr);
	cg->mov(cg->dword[cg->rsp + 16], cg->eax);

	Xbyak::Label block_compiled;

	cg->cmp(cg->dword[cg->rsp + 20], 20);
	cg->je(block_compiled);
	cg->mov(isBranchDelayed, cg->dword[cg->rsp + 8]);
	cg->cmp(isBranchDelayed, 1);
	cg->jge(block_compiled);

	cg->jmp(compile_instr);

	cg->L(block_compiled);

	cg->mov(cg->edi, cg->dword[cg->rsp + 20]);
	cg->mov(func_ptr, reinterpret_cast<uint64_t>(EmotionEngine::EmitDone));
	cg->call(func_ptr);

	cg->call(cg->rax);
	cg->mov(cg->rdi, cg->dword[cg->rsp + 20]);
	cg->mov(func_ptr, reinterpret_cast<uint64_t>(Scheduler::CheckScheduler));
	cg->call(func_ptr);

	cg->jmp(begin);

	cg->L(block);

	cg->mov(cg->rdi, cg->dword[cg->rsp]);
	cg->mov(func_ptr, reinterpret_cast<uint64_t>(EmotionEngine::GetExistingBlockFunc));
	cg->call(func_ptr);
	cg->call(cg->rax);

	cg->mov(cg->rdi, cg->dword[cg->rsp]);
	cg->mov(func_ptr, reinterpret_cast<uint64_t>(EmotionEngine::GetExistingBlockCycles));
	cg->call(func_ptr);
	cg->mov(cg->rdi, cg->rax);
	cg->mov(func_ptr, reinterpret_cast<uint64_t>(Scheduler::CheckScheduler));
	cg->call(func_ptr);

	cg->jmp(begin);
	
	cg->add(cg->rsp, 32);
	EmitRestoreHostRegs();

	dispatcher_entry = (EE_JIT::JIT::EntryFunc)base;
	base += cg->getSize();

	Dump();
}

void EE_JIT::Emitter::Dump()
{
	std::ofstream dump("out.bin");
	
	for (size_t i = 0; i < cg->getSize(); i++)
	{
		dump << cg->getCode()[i];
	}

	dump.flush();
	dump.close();

	munmap(base, 0xffffffff);
}

void EE_JIT::Emitter::TranslateBlock(Block *block)
{
	for (auto& i : block->ir)
	{
		EmitIR(i);
	}

	cg->ret();
}

void EE_JIT::Emitter::EnterDispatcher()
{
	dispatcher_entry();
}

const uint8_t *EE_JIT::Emitter::GetFreeBase()
{
	return cg->getCurr();
}
