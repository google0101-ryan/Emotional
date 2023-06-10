// (c) Copyright 2022 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include "emit.h"
#include <sys/mman.h>
#include <emu/cpu/ee/EmotionEngine.h>
#include <emu/memory/Bus.h>
#include <emu/sched/scheduler.h>
#include <fstream>
#include <cassert>
#include <emu/cpu/ee/dmac.hpp>
#include <emu/cpu/ee/vu.h>

EE_JIT::Emitter* EE_JIT::emit;

float convert(uint32_t value)
{
    switch(value & 0x7F800000)
    {
        case 0x0:
            value &= 0x80000000;
            return *(float*)&value;
        case 0x7F800000:
            value = (value & 0x80000000)|0x7F7FFFFF;
            return *(float*)&value;
        default:
            return *(float*)&value;
    }
}

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
	else if (i.args[1].IsCop1()) // Mfc1
	{
		Xbyak::Reg64 ee_reg_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 cop_reg_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg32 cop_reg_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[0].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u32)));
		auto src_offset = ((offsetof(EmotionEngine::ProcessorState, fprs)) + (i.args[1].GetReg() * sizeof(float)));
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
			auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, cop0_regs)) + (i.args[0].GetReg() * sizeof(float)));
			cg->lea(cop_reg_ptr, cg->ptr[cg->rbp + dest_offset]);
			cg->lea(ee_reg_ptr, cg->ptr[cg->rbp + src_offset]);
			cg->mov(ee_reg_value, cg->dword[ee_reg_ptr]);
			cg->mov(cg->dword[cop_reg_ptr], ee_reg_value);
		}
		else if (i.args[0].IsCop1())
		{
			Xbyak::Reg64 ee_reg_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
			Xbyak::Reg64 cop_reg_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
			Xbyak::Reg32 ee_reg_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
			auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[1].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u32)));
			auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, fprs)) + (i.args[0].GetReg() * sizeof(uint32_t)));
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

void EE_JIT::Emitter::EmitPMFLO(IRInstruction i)
{
	Xbyak::Reg64 ee_reg_lo_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg64 ee_reg_hi_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg64 ee_lo0_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg64 ee_lo1_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg64 tmp = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	
	auto dst_offset_lo = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[0].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u64)));
	auto dst_offset_hi = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[0].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u64)) + sizeof(uint64_t));
	auto lo0_offs = ((offsetof(EmotionEngine::ProcessorState, lo)));
	auto lo1_offs = ((offsetof(EmotionEngine::ProcessorState, lo1)));
	cg->lea(ee_reg_lo_ptr, cg->ptr[cg->rbp + dst_offset_lo]);
	cg->lea(ee_reg_hi_ptr, cg->ptr[cg->rbp + dst_offset_hi]);

	cg->mov(tmp, cg->qword[cg->rbp + lo0_offs]);
	cg->mov(cg->qword[ee_reg_lo_ptr], tmp);
	cg->mov(tmp, cg->qword[cg->rbp + lo1_offs]);
	cg->mov(cg->qword[ee_reg_hi_ptr], tmp);
}

void EE_JIT::Emitter::EmitPMFHI(IRInstruction i)
{
	Xbyak::Reg64 ee_reg_lo_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg64 ee_reg_hi_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg64 ee_hi0_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg64 ee_hi1_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg64 tmp = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	
	auto dst_offset_lo = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[0].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u64)));
	auto dst_offset_hi = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[0].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u64)) + sizeof(uint64_t));
	auto hi0_offs = ((offsetof(EmotionEngine::ProcessorState, hi)));
	auto hi1_offs = ((offsetof(EmotionEngine::ProcessorState, hi1)));
	cg->lea(ee_reg_lo_ptr, cg->ptr[cg->rbp + dst_offset_lo]);
	cg->lea(ee_reg_hi_ptr, cg->ptr[cg->rbp + dst_offset_hi]);

	cg->mov(tmp, cg->qword[cg->rbp + hi0_offs]);
	cg->mov(cg->qword[ee_reg_lo_ptr], tmp);
	cg->mov(tmp, cg->qword[cg->rbp + hi1_offs]);
	cg->mov(cg->qword[ee_reg_hi_ptr], tmp);
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
		cg->je(end);
	}
	else
	{
		cg->jne(end);
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
		cg->mov(src1_value, cg->qword[src1_ptr]);
		cg->lea(src2_ptr, cg->ptr[cg->rbp + src2_offset]);
		cg->mov(src2_value, cg->qword[src2_ptr]);
		cg->and_(src1_value, src2_value);
		cg->lea(dest_ptr, cg->ptr[cg->rbp + dest_offset]);
		cg->mov(cg->qword[dest_ptr], src1_value);
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

void PrintAboutStrnchr()
{
	if (EmotionEngine::GetState()->next_pc == 0x8000d638)
	{
		printf("strnchr(0x%08x, %d, %d)\n", EmotionEngine::GetState()->regs[4].u32[0], EmotionEngine::GetState()->regs[5].u32[0], EmotionEngine::GetState()->regs[6].u32[0]);
		exit(1);
	}
	if (EmotionEngine::GetState()->next_pc == 0x8000d550)
	{
		printf("memcpy(0x%08x, 0x%08x, %d)\n", EmotionEngine::GetState()->regs[4].u32[0], EmotionEngine::GetState()->regs[5].u32[0], EmotionEngine::GetState()->regs[6].u32[0]);
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
	
	// cg->mov(cg->rax, reinterpret_cast<uint64_t>(PrintAboutStrnchr));
	// cg->call(cg->rax);
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

			auto src1_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
			auto src2_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
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

void _LDL(uint32_t instruction)
{
	static const uint64_t LDL_MASK[8] =
    {	0x00ffffffffffffffULL, 0x0000ffffffffffffULL, 0x000000ffffffffffULL, 0x00000000ffffffffULL,
        0x0000000000ffffffULL, 0x000000000000ffffULL, 0x00000000000000ffULL, 0x0000000000000000ULL
    };
    static const uint8_t LDL_SHIFT[8] = { 56, 48, 40, 32, 24, 16, 8, 0 };
	int16_t imm = (int16_t)(instruction & 0xFFFF);
    uint64_t dest = (instruction >> 16) & 0x1F;
    uint64_t base = (instruction >> 21) & 0x1F;

	uint32_t addr = EmotionEngine::GetState()->regs[base].u32[0] + imm;
	uint32_t shift = addr & 0x7;

	uint64_t mem = Bus::Read64(addr & ~0x7);
	uint64_t reg = EmotionEngine::GetState()->regs[dest].u64[0];
	EmotionEngine::GetState()->regs[dest].u64[0] = (reg & LDL_MASK[shift]) | (mem << LDL_SHIFT[shift]);
}

void EE_JIT::Emitter::EmitLDL(IRInstruction i)
{
	cg->mov(cg->edi, i.opcode);
	cg->mov(cg->rax, reinterpret_cast<uint64_t>(_LDL));
	cg->call(cg->rax);
}

void _LDR(uint32_t instruction)
{
	static const uint64_t LDR_MASK[8] =
    {	0x0000000000000000ULL, 0xff00000000000000ULL, 0xffff000000000000ULL, 0xffffff0000000000ULL,
        0xffffffff00000000ULL, 0xffffffffff000000ULL, 0xffffffffffff0000ULL, 0xffffffffffffff00ULL
    };
    static const uint8_t LDR_SHIFT[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };
    int16_t imm = (int16_t)(instruction & 0xFFFF);
    uint64_t dest = (instruction >> 16) & 0x1F;
    uint64_t base = (instruction >> 21) & 0x1F;

	uint32_t addr = EmotionEngine::GetState()->regs[base].u32[0] + imm;
	uint32_t shift = addr & 0x7;

	uint64_t mem = Bus::Read64(addr & ~0x7);
	uint64_t reg = EmotionEngine::GetState()->regs[dest].u64[0];
	EmotionEngine::GetState()->regs[dest].u64[0] = (reg & LDR_MASK[shift]) | (mem >> LDR_SHIFT[shift]);
}

void EE_JIT::Emitter::EmitLDR(IRInstruction i)
{
	cg->mov(cg->edi, i.opcode);
	cg->mov(cg->rax, reinterpret_cast<uint64_t>(_LDR));
	cg->call(cg->rax);
}

void _SDL(uint32_t cur_instr)
{
	static const uint64_t SDL_MASK[8] =
    {	0xffffffffffffff00ULL, 0xffffffffffff0000ULL, 0xffffffffff000000ULL, 0xffffffff00000000ULL,
        0xffffff0000000000ULL, 0xffff000000000000ULL, 0xff00000000000000ULL, 0x0000000000000000ULL
    };
    static const uint8_t SDL_SHIFT[8] = { 56, 48, 40, 32, 24, 16, 8, 0 };
	int16_t imm = (int16_t)(cur_instr & 0xFFFF);
	uint64_t source = (cur_instr >> 16) & 0x1F;
    uint64_t base = (cur_instr >> 21) & 0x1F;

	uint32_t addr = EmotionEngine::GetState()->regs[base].u32[0] + imm;

	uint32_t shift = addr & 0x7;

	uint64_t mem = Bus::Read64(addr & ~0x7);
	mem = (EmotionEngine::GetState()->regs[source].u64[0] >> SDL_SHIFT[shift]) | (mem & SDL_MASK[shift]);
	Bus::Write64(addr & ~0x7, mem);
}

void EE_JIT::Emitter::EmitSDL(IRInstruction i)
{
	cg->mov(cg->edi, i.opcode);
	cg->mov(cg->rax, reinterpret_cast<uint64_t>(_SDL));
	cg->call(cg->rax);
}

void _SDR(uint32_t instruction)
{
	static const uint64_t SDR_MASK[8] =
    {	0x0000000000000000ULL, 0x00000000000000ffULL, 0x000000000000ffffULL, 0x0000000000ffffffULL,
        0x00000000ffffffffULL, 0x000000ffffffffffULL, 0x0000ffffffffffffULL, 0x00ffffffffffffffULL
    };
    static const uint8_t SDR_SHIFT[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };
    int16_t imm = (int16_t)(instruction & 0xFFFF);
    uint64_t source = (instruction >> 16) & 0x1F;
    uint64_t base = (instruction >> 21) & 0x1F;

    uint32_t addr = EmotionEngine::GetState()->regs[base].u32[0] + imm;
    uint32_t shift = addr & 0x7;

    uint64_t mem = Bus::Read64(addr & ~0x7);
    mem = (EmotionEngine::GetState()->regs[source].u64[0] << SDR_SHIFT[shift]) |
            (mem & SDR_MASK[shift]);
    Bus::Write64(addr & ~0x7, mem);
}

void EE_JIT::Emitter::EmitSDR(IRInstruction i)
{
	cg->mov(cg->edi, i.opcode);
	cg->mov(cg->rax, reinterpret_cast<uint64_t>(_SDR));
	cg->call(cg->rax);
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
		cg->imul(src2);

		cg->mov(cg->dword[cg->rbp + hi_offset], Xbyak::Reg32(RegisterAllocator::RDX));
		cg->mov(cg->dword[cg->rbp + lo_offset], Xbyak::Reg32(RegisterAllocator::RAX));
		cg->mov(cg->dword[cg->rbp + dest_offset], Xbyak::Reg32(RegisterAllocator::RAX));
	}
	else
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
	auto hi_offset = i.is_mmi_divmul ? ((offsetof(EmotionEngine::ProcessorState, hi1))) : ((offsetof(EmotionEngine::ProcessorState, hi)));

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

	Xbyak::Label end;

	if (i.should_link)
	{
		Xbyak::Reg32 next_pc_val = Xbyak::Reg32(reg_alloc->AllocHostRegister());
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (31 * sizeof(uint128_t)) + offsetof(uint128_t, u32)));


		cg->mov(next_pc_val, cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)]);
		cg->mov(cg->dword[cg->rbp + dest_offset], next_pc_val);
	}
	cg->mov(next_pc, cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, pc)]);
	cg->add(next_pc, i.args[1].GetImm());
	cg->mov(cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)], next_pc);

	cg->jmp(end);

	cg->L(cond_failed);

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

void EE_JIT::Emitter::EmitPADDUSW(IRInstruction i)
{
	Xbyak::Xmm src1 = Xbyak::Xmm(reg_alloc->AllocHostXMMRegister());
	Xbyak::Xmm src2 = Xbyak::Xmm(reg_alloc->AllocHostXMMRegister());
	auto reg1_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u128)));
	auto reg2_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u128)));
	auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u128)));
	
	cg->movaps(src1, cg->ptr[cg->rbp + reg1_offset]);
	cg->movaps(src2, cg->ptr[cg->rbp + reg2_offset]);

	cg->paddusw(src1, src2);

	cg->movaps(cg->ptr[cg->rbp + dest_offset], src1);
}

void EE_JIT::Emitter::EmitDI(IRInstruction i)
{
	Xbyak::Reg32 stat_reg = Xbyak::Reg32(reg_alloc->AllocHostRegister());
	Xbyak::Reg32 intermediate_reg = Xbyak::Reg32(reg_alloc->AllocHostRegister());

	auto stat_offset = ((offsetof(EmotionEngine::ProcessorState, cop0_regs) + (sizeof(uint32_t) * 12)));

	cg->mov(stat_reg, cg->dword[cg->rbp + stat_offset]);
	cg->mov(intermediate_reg, stat_reg);

	Xbyak::Label set, end;

	cg->and_(intermediate_reg, (1 << 17));
	cg->je(set);
	cg->mov(intermediate_reg, stat_reg);
	cg->and_(intermediate_reg, (1 << 1));
	cg->je(set);
	cg->mov(intermediate_reg, stat_reg);
	cg->and_(intermediate_reg, (1 << 2));
	cg->je(set);

	cg->jmp(end);

	cg->L(set);

	cg->mov(intermediate_reg, stat_reg);

	cg->and_(stat_reg, ~(1 << 16));
	cg->mov(cg->dword[cg->rbp + stat_offset], stat_reg);

	cg->L(end);
}

bool elf_loaded = false;

void HandleERET()
{
	uint32_t status = EmotionEngine::GetState()->cop0_regs[12];

	// EmotionEngine::can_disassemble = true;

	if (status & (1 << 2))
	{
		EmotionEngine::GetState()->cop0_regs[12] &= ~(1 << 2);
		EmotionEngine::GetState()->pc = EmotionEngine::GetState()->cop0_regs[30];
		EmotionEngine::GetState()->next_pc = EmotionEngine::GetState()->cop0_regs[30]+4;
	}
	else
	{
		EmotionEngine::GetState()->cop0_regs[12] &= ~(1 << 1);
		EmotionEngine::GetState()->pc = EmotionEngine::GetState()->cop0_regs[14];
		EmotionEngine::GetState()->next_pc = EmotionEngine::GetState()->cop0_regs[14]+4;
	}
		
	if (!elf_loaded)
	{
		uint32_t entry = Bus::LoadElf("graph.elf");
	 	EmotionEngine::GetState()->pc = entry;
	 	EmotionEngine::GetState()->next_pc = entry+4;
	 	elf_loaded = true;
	}
}

void EE_JIT::Emitter::EmitERET(IRInstruction i)
{
	cg->mov(cg->rax, reinterpret_cast<uint64_t>(HandleERET));
	cg->call(cg->rax);
	reg_alloc->Reset();
	// EmitIncPC(i);
	cg->ret();
}

void EE_JIT::Emitter::EmitSyscall(IRInstruction i)
{
	cg->mov(cg->edi, 0x08);
	cg->mov(cg->rax, reinterpret_cast<uint64_t>(EmotionEngine::Exception));
	cg->call(cg->rax);
	reg_alloc->Reset();
	// EmitIncPC(i);
	cg->ret();
}

void EE_JIT::Emitter::EmitEI(IRInstruction i)
{
	Xbyak::Reg32 stat_reg = Xbyak::Reg32(reg_alloc->AllocHostRegister());
	Xbyak::Reg32 intermediate_reg = Xbyak::Reg32(reg_alloc->AllocHostRegister());

	auto stat_offset = ((offsetof(EmotionEngine::ProcessorState, cop0_regs) + (sizeof(uint32_t) * 12)));

	cg->mov(stat_reg, cg->dword[cg->rbp + stat_offset]);
	cg->mov(intermediate_reg, stat_reg);

	Xbyak::Label set, end;

	cg->and_(intermediate_reg, (1 << 17));
	cg->jne(set);
	cg->mov(intermediate_reg, stat_reg);
	cg->and_(intermediate_reg, (1 << 1));
	cg->jne(set);
	cg->mov(intermediate_reg, stat_reg);
	cg->and_(intermediate_reg, (1 << 2));
	cg->jne(set);

	cg->jmp(end);

	cg->L(set);

	cg->mov(intermediate_reg, stat_reg);

	cg->or_(stat_reg, 1 << 16);
	cg->mov(cg->dword[cg->rbp + stat_offset], stat_reg);

	cg->L(end);
}

void plzcw(int rs, int rd)
{
	auto state = EmotionEngine::GetState();

	for (int i = 0; i < 2; i++)
	{
		uint32_t word = state->regs[rs].u32[i];
		bool msb = word & (1 << 31);

		word = (msb ? ~word : word);

		state->regs[rd].u32[i] = (word != 0 ? __builtin_clz(word) - 1 : 0x1f);
	}

	printf("plzcw (0x%08lx)\n", state->regs[rd].u64[0]);
}

void EE_JIT::Emitter::EmitPLZCW(IRInstruction i)
{
	assert(i.args.size() == 2 && i.args[0].IsReg() && i.args[1].IsReg());
	cg->mov(cg->edi, i.args[1].GetReg());
	cg->mov(cg->esi, i.args[0].GetReg());
	cg->mov(cg->rax, reinterpret_cast<uint64_t>(plzcw));
	cg->call(cg->rax);
}

void EE_JIT::Emitter::EmitMTLO(IRInstruction i)
{
	Xbyak::Reg64 reg_val = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	auto src_offs = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
	auto lo_offset = i.is_mmi_divmul ? ((offsetof(EmotionEngine::ProcessorState, lo1))) : ((offsetof(EmotionEngine::ProcessorState, lo)));
	cg->mov(reg_val, cg->qword[cg->rbp + src_offs]);
	cg->mov(cg->dword[cg->rbp + lo_offset], reg_val);
}

void EE_JIT::Emitter::EmitMTHI(IRInstruction i)
{
	Xbyak::Reg64 reg_val = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	auto src_offs = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
	auto hi_offset = i.is_mmi_divmul ? ((offsetof(EmotionEngine::ProcessorState, lo1))) : ((offsetof(EmotionEngine::ProcessorState, lo)));
	cg->mov(reg_val, cg->qword[cg->rbp + src_offs]);
	cg->mov(cg->dword[cg->rbp + hi_offset], reg_val);
}

void EE_JIT::Emitter::EmitPCPYLD(IRInstruction i)
{
	Xbyak::Reg64 reg_val = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	auto src1_offs = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
	auto src2_offs = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
	auto dst_offs = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
	
	cg->mov(reg_val, cg->dword[cg->rbp + src2_offs]);
	cg->mov(cg->dword[cg->rbp + dst_offs], reg_val);
	cg->mov(reg_val, cg->dword[cg->rbp + src1_offs]);
	cg->mov(cg->dword[cg->rbp + dst_offs + 8], reg_val);
}

void EE_JIT::Emitter::EmitPSUBB(IRInstruction i)
{
	Xbyak::Xmm reg1 = Xbyak::Xmm(reg_alloc->AllocHostXMMRegister());
	Xbyak::Xmm reg2 = Xbyak::Xmm(reg_alloc->AllocHostXMMRegister());
	auto reg1_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[0].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u64)));
	auto reg2_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[1].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u64)));
	auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[2].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u64)));

	cg->movaps(reg1, cg->ptr[cg->rbp + reg1_offset]);
	cg->movaps(reg2, cg->ptr[cg->rbp + reg2_offset]);

	cg->psubb(reg1, reg2);
	cg->movaps(cg->ptr[cg->rbp + dest_offset], reg1);
}

void EE_JIT::Emitter::EmitPADDSB(IRInstruction i)
{
	Xbyak::Xmm reg1 = Xbyak::Xmm(reg_alloc->AllocHostXMMRegister());
	Xbyak::Xmm reg2 = Xbyak::Xmm(reg_alloc->AllocHostXMMRegister());
	auto reg1_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[0].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u64)));
	auto reg2_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[1].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u64)));
	auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[2].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u64)));

	cg->movaps(reg1, cg->ptr[cg->rbp + reg1_offset]);
	cg->movaps(reg2, cg->ptr[cg->rbp + reg2_offset]);

	cg->paddsb(reg1, reg2);

	cg->movaps(cg->ptr[cg->rbp + dest_offset], reg1);
}

void EE_JIT::Emitter::EmitPNOR(IRInstruction i)
{
	Xbyak::Xmm src1 = Xbyak::Xmm(reg_alloc->AllocHostXMMRegister());
	Xbyak::Xmm src2 = Xbyak::Xmm(reg_alloc->AllocHostXMMRegister());
	Xbyak::Xmm tmp = Xbyak::Xmm(reg_alloc->AllocHostXMMRegister());
	auto reg1_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u128)));
	auto reg2_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u128)));
	auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u128)));
	
	cg->movaps(src1, cg->ptr[cg->rbp + reg1_offset]);
	cg->movaps(src2, cg->ptr[cg->rbp + reg2_offset]);

	cg->pcmpeqd(tmp, tmp);
	cg->pxor(src2, tmp);

	cg->por(src1, src2);

	cg->movaps(cg->ptr[cg->rbp + dest_offset], src1);
}

void EE_JIT::Emitter::EmitPAND(IRInstruction i)
{
	Xbyak::Xmm src1 = Xbyak::Xmm(reg_alloc->AllocHostXMMRegister());
	Xbyak::Xmm src2 = Xbyak::Xmm(reg_alloc->AllocHostXMMRegister());
	Xbyak::Xmm tmp = Xbyak::Xmm(reg_alloc->AllocHostXMMRegister());
	auto reg1_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u128)));
	auto reg2_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u128)));
	auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u128)));
	
	cg->movaps(src1, cg->ptr[cg->rbp + reg1_offset]);
	cg->movaps(src2, cg->ptr[cg->rbp + reg2_offset]);

	cg->pand(src1, src2);

	cg->movaps(cg->ptr[cg->rbp + dest_offset], src1);
}

void EE_JIT::Emitter::EmitPCPYUD(IRInstruction i)
{
	Xbyak::Reg64 reg_val = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	auto src1_offs = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64) + sizeof(uint64_t)));
	auto src2_offs = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64) + sizeof(uint64_t)));
	auto dst_offs = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
	
	cg->mov(reg_val, cg->dword[cg->rbp + src2_offs]);
	cg->mov(cg->dword[cg->rbp + dst_offs], reg_val);
	cg->mov(reg_val, cg->dword[cg->rbp + src1_offs]);
	cg->mov(cg->dword[cg->rbp + dst_offs + 8], reg_val);
}

bool GetCPCOND0()
{
	return DMAC::GetCPCOND0();
}

void EE_JIT::Emitter::EmitBC0T(IRInstruction i)
{
	Xbyak::Reg64 tmp = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg32 pc = Xbyak::Reg32(reg_alloc->AllocHostRegister());

	Xbyak::Label condition_failed;
	Xbyak::Label end;
	
	cg->mov(tmp, reinterpret_cast<uint64_t>(GetCPCOND0));
	cg->call(tmp);
	cg->test(cg->al, cg->al);
	cg->je(end);
	cg->mov(pc, cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, pc)]);
	cg->add(pc, i.args[0].GetImm());
	cg->mov(cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)], pc);
	
	cg->L(end);
}

void EE_JIT::Emitter::EmitBC0F(IRInstruction i)
{
	Xbyak::Reg64 tmp = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg32 pc = Xbyak::Reg32(reg_alloc->AllocHostRegister());

	Xbyak::Label condition_failed;
	Xbyak::Label end;
	
	cg->mov(tmp, reinterpret_cast<uint64_t>(GetCPCOND0));
	cg->call(tmp);
	cg->test(cg->al, cg->al);
	cg->jne(end);
	cg->mov(pc, cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, pc)]);
	cg->add(pc, i.args[0].GetImm());
	cg->mov(cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)], pc);
	
	cg->L(end);
}

void EE_JIT::Emitter::EmitPCPYH(IRInstruction i)
{
	Xbyak::Reg16 reg_val = Xbyak::Reg16(reg_alloc->AllocHostRegister());
	auto src1_offs = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
	auto dst_offs = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
	
	cg->mov(reg_val, cg->word[cg->rbp+src1_offs]);
	for (int i = 0; i < 4; i++)
		cg->mov(cg->word[cg->rbp+dst_offs+(i*2)], reg_val);
	cg->mov(reg_val, cg->word[cg->rbp+src1_offs+sizeof(uint64_t)]);
	for (int i = 0; i < 4; i++)
		cg->mov(cg->word[cg->rbp+dst_offs+sizeof(uint64_t)+(i*2)], reg_val);
}

void EE_JIT::Emitter::EmitCFC2(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	auto dst_offs = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));

	cg->mov(cg->rdi, i.args[1].GetReg());
	cg->mov(fptr, reinterpret_cast<uint64_t>(VectorUnit::VU0::ReadControl));
	cg->mov(cg->dword[cg->rbp+dst_offs], cg->eax);
}

void EE_JIT::Emitter::EmitCTC2(IRInstruction i)
{
	reg_alloc->MarkRegUsed(RegisterAllocator::RDI);
	reg_alloc->MarkRegUsed(RegisterAllocator::RSI);
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	auto src_offs = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));

	cg->mov(cg->rdi, i.args[1].GetReg());
	cg->mov(cg->esi, cg->dword[cg->rbp+src_offs]);
	cg->mov(fptr, reinterpret_cast<uint64_t>(VectorUnit::VU0::WriteControl));
	cg->call(fptr);
}

void EE_JIT::Emitter::EmitVISWR(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(VectorUnit::VU0::Viswr));
	cg->call(fptr);
}

void EE_JIT::Emitter::EmitQMFC2(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	auto dst_offs = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));

	cg->mov(cg->rdi, i.args[1].GetReg());
	cg->mov(fptr, reinterpret_cast<uint64_t>(VectorUnit::VU0::ReadReg));
	cg->mov(cg->qword[cg->rbp+dst_offs], cg->rax);
	cg->mov(cg->qword[cg->rbp+dst_offs+sizeof(uint64_t)], cg->rdx);
}

void EE_JIT::Emitter::EmitQMTC2(IRInstruction i)
{
	reg_alloc->MarkRegUsed(RegisterAllocator::RDI);
	reg_alloc->MarkRegUsed(RegisterAllocator::RSI);
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	auto src_offs = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));

	cg->mov(cg->rdi, i.args[1].GetReg());
	cg->mov(cg->rsi, cg->qword[cg->rbp+src_offs]);
	cg->mov(cg->rdx, cg->qword[cg->rbp+src_offs+sizeof(uint64_t)]);
	cg->mov(fptr, reinterpret_cast<uint64_t>(VectorUnit::VU0::WriteReg));
	cg->call(fptr);
}

void EE_JIT::Emitter::EmitVSUB(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(VectorUnit::VU0::Vsub));
	cg->call(fptr);
}

void EE_JIT::Emitter::EmitVSQI(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(VectorUnit::VU0::Vsqi));
	cg->call(fptr);
}

void EE_JIT::Emitter::EmitVIADD(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(VectorUnit::VU0::Viadd));
	cg->call(fptr);
}

void DoADDAS(uint32_t instr)
{
	auto state = EmotionEngine::GetState();

	uint8_t ft = (instr >> 16) & 0x1F;
	uint8_t fs = (instr >> 11) & 0x1F;

	state->acc.f = convert(state->fprs[fs].i) + convert(state->fprs[ft].i);
}

void EE_JIT::Emitter::EmitADDA(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(DoADDAS));
	cg->call(fptr);
}

void DoMADD(uint32_t instr)
{
	uint8_t rs = (instr >> 21) & 0x1F;
	uint8_t rt = (instr >> 16) & 0x1F;
	uint8_t rd = (instr >> 11) & 0x1F;

	uint64_t res = ((int32_t)EmotionEngine::GetState()->regs[rs].u32[0]) * ((int32_t)EmotionEngine::GetState()->regs[rt].u32[0]);
	uint64_t hilo = (EmotionEngine::GetState()->hi << 32) | EmotionEngine::GetState()->lo;

	res += hilo;

	EmotionEngine::GetState()->hi = (res >> 32);
	EmotionEngine::GetState()->lo = EmotionEngine::GetState()->regs[rd].u32[0] = (res & 0xffffffff);
}

void EE_JIT::Emitter::EmitMADD(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(DoMADD));
	cg->call(fptr);
}

void DoMADDS(uint32_t instr)
{
	uint8_t ft = (instr >> 16) & 0x1F;
	uint8_t fs = (instr >> 11) & 0x1F;
	uint8_t fd = (instr >> 6) & 0x1F;

	auto state = EmotionEngine::GetState();

	state->fprs[fd].f = convert(state->acc.u) + (convert(state->fprs[fs].i) * convert(state->fprs[ft].i));
}

void EE_JIT::Emitter::EmitMADDS(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(DoMADDS));
	cg->call(fptr);
}

void DoCVTS(uint32_t instr)
{
	uint8_t fs = (instr >> 11) & 0x1F;
	uint8_t fd = (instr >> 6) & 0x1F;

	auto state = EmotionEngine::GetState();
	
	if ((state->fprs[fs].i & 0x7F800000) <= 0x4E800000)
		state->fprs[fd].s = (int32_t)state->fprs[fs].f;
	else if ((state->fprs[fs].i & 0x80000000) == 0)
		state->fprs[fd].i = 0x7FFFFFFF;
	else
		state->fprs[fd].i = 0x80000000;
}

void EE_JIT::Emitter::EmitCVTS(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(DoCVTS));
	cg->call(fptr);
}

void DoMULS(uint32_t instr)
{
	uint8_t ft = (instr >> 16) & 0x1F;
	uint8_t fs = (instr >> 11) & 0x1F;
	uint8_t fd = (instr >> 6) & 0x1F;

	auto state = EmotionEngine::GetState();

	state->fprs[fd].f = state->fprs[fs].f * state->fprs[ft].f;
}

void EE_JIT::Emitter::EmitMULS(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(DoMULS));
	cg->call(fptr);
}

void DoCVTW(uint32_t instr)
{
	uint8_t fs = (instr >> 11) & 0x1F;
	uint8_t fd = (instr >> 6) & 0x1F;

	auto state = EmotionEngine::GetState();
	
	state->fprs[fd].f = (float)state->fprs[fs].s;
	state->fprs[fd].f = convert(state->fprs[fd].i);
}

void EE_JIT::Emitter::EmitCVTW(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(DoCVTW));
	cg->call(fptr);
}

void DoDIVS(uint32_t instr)
{
	uint8_t ft = (instr >> 16) & 0x1F;
	uint8_t fs = (instr >> 11) & 0x1F;
	uint8_t fd = (instr >> 6) & 0x1F;

	auto state = EmotionEngine::GetState();

	if ((state->fprs[ft].i & 0x7F800000) == 0)
		state->fprs[fd].i = ((state->fprs[fs].i ^ state->fprs[ft].i) & 0x80000000) | 0x7F7FFFFF;
	else
		state->fprs[fd].f = convert(state->fprs[fs].i) / convert(state->fprs[ft].i);
}

void EE_JIT::Emitter::EmitDIVS(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(DoDIVS));
	cg->call(fptr);
}

void DoMOVS(uint32_t instr)
{
	uint8_t fs = (instr >> 11) & 0x1F;
	uint8_t fd = (instr >> 6) & 0x1F;

	auto state = EmotionEngine::GetState();

	state->fprs[fd].i = state->fprs[fs].i;
}

void EE_JIT::Emitter::EmitMOVS(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(DoMOVS));
	cg->call(fptr);
}

void DoADDS(uint32_t instr)
{
	uint8_t ft = (instr >> 16) & 0x1F;
	uint8_t fs = (instr >> 11) & 0x1F;
	uint8_t fd = (instr >> 6) & 0x1F;

	auto state = EmotionEngine::GetState();

	state->fprs[fd].f = (convert(state->fprs[fs].i) + convert(state->fprs[ft].i));
}

void EE_JIT::Emitter::EmitADDS(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(DoADDS));
	cg->call(fptr);
}

void DoSUBS(uint32_t instr)
{
	uint8_t ft = (instr >> 16) & 0x1F;
	uint8_t fs = (instr >> 11) & 0x1F;
	uint8_t fd = (instr >> 6) & 0x1F;

	auto state = EmotionEngine::GetState();

	state->fprs[fd].f = (convert(state->fprs[fs].i) - convert(state->fprs[ft].i));
}

void EE_JIT::Emitter::EmitSUBS(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(DoSUBS));
	cg->call(fptr);
}

void DoNEGS(uint32_t instr)
{
	uint8_t fs = (instr >> 11) & 0x1F;
	uint8_t fd = (instr >> 6) & 0x1F;

	auto state = EmotionEngine::GetState();

	state->fprs[fd].i = state->fprs[fs].i ^ 0x80000000;
}

void EE_JIT::Emitter::EmitNEGS(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(DoNEGS));
	cg->call(fptr);
}

void EE_JIT::Emitter::EmitLQC2(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(VectorUnit::VU0::LQC2));
	cg->call(fptr);
}

void EE_JIT::Emitter::EmitSQC2(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(VectorUnit::VU0::SQC2));
	cg->call(fptr);
}

void EE_JIT::Emitter::EmitVMULAX(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(VectorUnit::VU0::Vmulax));
	cg->call(fptr);
}

void EE_JIT::Emitter::EmitVMADDAX(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(VectorUnit::VU0::Vmaddax));
	cg->call(fptr);
}

void EE_JIT::Emitter::EmitVMADDAY(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(VectorUnit::VU0::Vmadday));
	cg->call(fptr);
}

void EE_JIT::Emitter::EmitVMADDAZ(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(VectorUnit::VU0::Vmaddaz));
	cg->call(fptr);
}

void EE_JIT::Emitter::EmitVMADDW(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(VectorUnit::VU0::Vmaddw));
	cg->call(fptr);
}

void EE_JIT::Emitter::EmitVMADDZ(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(VectorUnit::VU0::Vmaddz));
	cg->call(fptr);
}

void EE_JIT::Emitter::EmitVMULAW(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(VectorUnit::VU0::Vmulaw));
	cg->call(fptr);
}

void EE_JIT::Emitter::EmitVCLIPW(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(VectorUnit::VU0::Vclipw));
	cg->call(fptr);
}

void DoCLES(uint32_t instr)
{
	uint8_t ft = (instr >> 16) & 0x1F;
	uint8_t fs = (instr >> 11) & 0x1F;

	if (convert(EmotionEngine::GetState()->fprs[fs].i) <= convert(EmotionEngine::GetState()->fprs[ft].i))
		EmotionEngine::GetState()->c = 1;
	else
		EmotionEngine::GetState()->c = 0;
}

void EE_JIT::Emitter::EmitCLES(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(DoCLES));
	cg->call(fptr);
}

void DoCEQS(uint32_t instr)
{
	uint8_t ft = (instr >> 16) & 0x1F;
	uint8_t fs = (instr >> 11) & 0x1F;

	if (convert(EmotionEngine::GetState()->fprs[fs].i) == convert(EmotionEngine::GetState()->fprs[ft].i))
		EmotionEngine::GetState()->c = 1;
	else
		EmotionEngine::GetState()->c = 0;
}

void EE_JIT::Emitter::EmitCEQS(IRInstruction i)
{
	Xbyak::Reg64 fptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	cg->mov(cg->rdi, i.args[0].GetImm());
	cg->mov(fptr, reinterpret_cast<uint64_t>(DoCEQS));
	cg->call(fptr);
}

bool GetFPUC()
{
	return EmotionEngine::GetState()->c;
}

void EE_JIT::Emitter::EmitBC1F(IRInstruction i)
{	
	Xbyak::Reg64 tmp = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg32 pc = Xbyak::Reg32(reg_alloc->AllocHostRegister());

	Xbyak::Label condition_failed;
	Xbyak::Label end;
	
	cg->mov(tmp, reinterpret_cast<uint64_t>(GetFPUC));
	cg->call(tmp);
	cg->test(cg->al, cg->al);
	cg->jne(end);
	cg->mov(pc, cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, pc)]);
	cg->add(pc, i.args[0].GetImm());
	cg->mov(cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)], pc);
	
	cg->L(end);
}

void EE_JIT::Emitter::EmitBC1T(IRInstruction i)
{
	Xbyak::Reg64 tmp = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg32 pc = Xbyak::Reg32(reg_alloc->AllocHostRegister());

	Xbyak::Label condition_failed;
	Xbyak::Label end;
	
	cg->mov(tmp, reinterpret_cast<uint64_t>(GetFPUC));
	cg->call(tmp);
	cg->test(cg->al, cg->al);
	cg->je(end);
	cg->mov(pc, cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, pc)]);
	cg->add(pc, i.args[0].GetImm());
	cg->mov(cg->dword[cg->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)], pc);
	
	cg->L(end);
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
	case LDL:
		EmitLDL(i);
		break;
	case LDR:
		EmitLDR(i);
		break;
	case SDL:
		EmitSDL(i);
		break;
	case SDR:
		EmitSDR(i);
		break;
	case PADDUSW:
		EmitPADDUSW(i);
		break;
	case DI:
		EmitDI(i);
		break;
	case EI:
		EmitEI(i);
		break;
	case ERET:
		EmitERET(i);
		break;
	case SYSCALL:
		EmitSyscall(i);
		break;
	case PLZCW:
		EmitPLZCW(i);
		break;
	case PMFHI:
		EmitPMFHI(i);
		break;
	case PMFLO:
		EmitPMFLO(i);
		break;
	case MTLO:
		EmitMTLO(i);
		break;
	case MTHI:
		EmitMTHI(i);
		break;
	case PCPYLD:
		EmitPCPYLD(i);
		break;
	case PSUBB:
		EmitPSUBB(i);
		break;
	case PNOR:
		EmitPNOR(i);
		break;
	case PAND:
		EmitPAND(i);
		break;
	case PCPYUD:
		EmitPCPYUD(i);
		break;
	case BC0T:
		EmitBC0T(i);
		break;
	case BC0F:
		EmitBC0F(i);
		break;
	case PCPYH:
		EmitPCPYH(i);
		break;
	case CFC2:
		EmitCFC2(i);
		break;
	case CTC2:
		EmitCTC2(i);
		break;
	case VISWR:
		EmitVISWR(i);
		break;
	case QMFC2:
		EmitQMFC2(i);
		break;
	case QMTC2:
		EmitQMTC2(i);
		break;
	case VSUB:
		EmitVSUB(i);
		break;
	case VSQI:
		EmitVSQI(i);
		break;
	case VIADD:
		EmitVIADD(i);
		break;
	case ADDAS:
		EmitADDA(i);
		break;
	case PADDSB:
		EmitPADDSB(i);
		break;
	case MADD:
		EmitMADD(i);
		break;
	case MADDS:
		EmitMADDS(i);
		break;
	case CVTS:
		EmitCVTS(i);
		break;
	case CVTW:
		EmitCVTW(i);
		break;
	case MULS:
		EmitMULS(i);
		break;
	case DIVS:
		EmitDIVS(i);
		break;
	case MOVS:
		EmitMOVS(i);
		break;
	case ADDS:
		EmitADDS(i);
		break;
	case SUBS:
		EmitSUBS(i);
		break;
	case NEGS:
		EmitNEGS(i);
		break;
	case LQC2:
		EmitLQC2(i);
		break;
	case SQC2:
		EmitSQC2(i);
		break;
	case VMULAX:
		EmitVMULAX(i);
		break;
	case VMADDAZ:
		EmitVMADDAZ(i);
		break;
	case VMADDAY:
		EmitVMADDAY(i);
		break;
	case VMADDW:
		EmitVMADDW(i);
		break;
	case VMADDZ:
		EmitVMADDZ(i);
		break;
	case VMADDAX:
		EmitVMADDAX(i);
		break;
	case VMULAW:
		EmitVMULAW(i);
		break;
	case VCLIPW:
		EmitVCLIPW(i);
		break;
	case CLES:
		EmitCLES(i);
		break;
	case BC1F:
		EmitBC1F(i);
		break;
	case BC1T:
		EmitBC1T(i);
		break;
	case CEQS:
		EmitCEQS(i);
		break;
	default:
		printf("[JIT/Emit]: Unknown IR instruction %d\n", i.instr);
		exit(1);
	}
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

void CheckInterrupts()
{
	COP0Status status;
	COP0CAUSE cause;

	status.value = EmotionEngine::GetState()->cop0_regs[12];
	cause.value = EmotionEngine::GetState()->cop0_regs[13];

	bool int_enabled = (status.eie && status.ie && !status.erl && !status.exl);

	bool pending = (cause.ip0_pending && status.im0) ||
					(cause.ip1_pending && status.im1) ||
					(cause.timer_ip_pending && status.im7);
	
	if (int_enabled && pending)
	{
		EmotionEngine::Exception(0x00);
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
	cg->mov(cg->dword[cg->rsp + 24], 0);

	Xbyak::Label begin;
	cg->L(begin);

	cg->mov(cg->dword[cg->rsp + 20], 0);

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
	cg->jge(block_compiled);
	cg->mov(isBranchDelayed, cg->dword[cg->rsp + 8]);
	cg->cmp(isBranchDelayed, 1);
	cg->jge(block_compiled);

	cg->jmp(compile_instr);

	cg->L(block_compiled);

	cg->mov(cg->edi, cg->dword[cg->rsp + 20]);
	cg->mov(func_ptr, reinterpret_cast<uint64_t>(EmotionEngine::EmitDone));
	cg->call(func_ptr);

	cg->call(cg->rax);

	cg->mov(cg->edi, cg->dword[cg->rsp + 20]);
	cg->mov(func_ptr, reinterpret_cast<uint64_t>(Scheduler::CheckScheduler));
	cg->call(func_ptr);
	cg->mov(func_ptr, reinterpret_cast<uint64_t>(CheckInterrupts));
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
	cg->mov(func_ptr, reinterpret_cast<uint64_t>(CheckInterrupts));
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

uint8_t *EE_JIT::Emitter::GetFreeBase()
{
	return (uint8_t*)cg->getCurr();
}
