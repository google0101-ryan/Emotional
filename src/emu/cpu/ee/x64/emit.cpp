// (c) Copyright 2022 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include "emit.h"
#include <sys/mman.h>
#include <emu/cpu/ee/EmotionEngine.h>
#include <emu/memory/Bus.h>
#include <fstream>
#include <cassert>

EE_JIT::Emitter* EE_JIT::emit;

void EE_JIT::Emitter::EmitMov(IRInstruction i)
{
	if (i.args[1].IsImm()) // Lui
	{
		Xbyak::Reg64 reg_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		auto offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[0].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u32)));
		cg->lea(reg_ptr, cg->ptr[cg->rbp + offset]);
		cg->mov(cg->dword[reg_ptr], i.args[1].GetImm());
	}
	else if (i.args[1].IsCop0()) // Mfc0
	{
		printf("%d, %d\n", i.args[0].GetReg(), i.args[1].GetReg());
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

void EE_JIT::Emitter::EmitSLTI(IRInstruction i)
{
	if (!i.args[2].IsImm())
	{
		printf("[emu/JIT]: Argument 2 to SLTI is not immediate\n");
		exit(1);
	}

	Xbyak::Reg64 ee_dest_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg64 ee_src_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	Xbyak::Reg64 ee_src_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[0].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u32)));
	auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[1].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u32)));
			
	cg->lea(ee_src_ptr, cg->ptr[cg->rbp + src_offset]);
	cg->mov(ee_src_value, cg->dword[ee_src_ptr]);
	cg->cmp(ee_src_value, i.args[1].GetImm());
	Xbyak::Label not_lt;
	Xbyak::Label done;
	cg->lea(ee_dest_ptr, cg->ptr[cg->rbp + dest_offset]);
	cg->jge(not_lt);
	cg->mov(cg->dword[ee_dest_ptr], 1);
	cg->jmp(done);
	cg->L(not_lt);
	cg->mov(cg->dword[ee_dest_ptr], 0);
	cg->L(done);
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
	Xbyak::Reg64 next_pc = Xbyak::Reg64(reg_alloc->AllocHostRegister());
	auto reg1_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[0].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u64)));
	auto reg2_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[1].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u64)));
	
	cg->lea(ee_reg1_ptr, cg->ptr[cg->rbp + reg1_offset]);
	cg->mov(ee_reg1_value, cg->dword[ee_reg1_ptr]);
	cg->lea(ee_reg2_ptr, cg->ptr[cg->rbp + reg2_offset]);
	cg->mov(ee_reg2_value, cg->dword[ee_reg2_ptr]);
	cg->cmp(ee_reg1_value, ee_reg2_value);
	Xbyak::Label conditional_failed;

	if (i.b_type == IRInstruction::NE)
	{
		cg->je(conditional_failed);
		cg->lea(next_pc, cg->ptr[cg->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)]);
		cg->add(cg->dword[next_pc], i.args[2].GetImm() << 2);
	}
	else if (i.b_type == IRInstruction::EQ)
	{
		cg->jne(conditional_failed);
		cg->lea(next_pc, cg->ptr[cg->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)]);
		cg->add(cg->dword[next_pc], i.args[2].GetImm() << 2);
	}
	else
	{
		printf("[emu/JIT]: Unknown branch conditional type %d\n", i.b_type);
		exit(1);
	}

	cg->L(conditional_failed);
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
		Xbyak::Reg32 src_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());

		auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));


		cg->lea(src_ptr, cg->ptr[cg->rbp + src_offset]);
		cg->mov(src_value, cg->dword[src_ptr]);
		cg->or_(src_value, i.args[2].GetImm());
		cg->lea(dest_ptr, cg->ptr[cg->rbp + dest_offset]);
		cg->mov(cg->dword[dest_ptr], src_value);
	}
	else
	{
		printf("[emu/JIT]: Unknown OR argument\n");
		exit(1);
	}
}

void EE_JIT::Emitter::EmitAND(IRInstruction i)
{
	if (i.args[2].IsImm())
	{
		Xbyak::Reg64 src_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 dest_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg32 src_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());

		auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));


		cg->lea(src_ptr, cg->ptr[cg->rbp + src_offset]);
		cg->mov(src_value, cg->dword[src_ptr]);
		cg->and_(src_value, i.args[2].GetImm());
		cg->lea(dest_ptr, cg->ptr[cg->rbp + dest_offset]);
		cg->mov(cg->dword[dest_ptr], src_value);
	}
	else
	{
		printf("[emu/JIT]: Unknown OR argument\n");
		exit(1);
	}
}

void EE_JIT::Emitter::EmitJA(IRInstruction i)
{

	if (i.should_link)
	{
		if (i.args.size() == 2)
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
		}
		else
		{
			printf("TODO: JAL support\n");
			exit(1);
		}
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
			auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
			auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));

			cg->mov(ee_src_value, cg->qword[cg->rbp + src_offset]);
			cg->add(ee_src_value, i.args[2].GetImm());
			cg->mov(cg->qword[cg->rbp + dest_offset], ee_src_value);
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
			
			auto src1_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
			auto src2_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
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
			
			auto src1_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
			auto src2_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
			auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));

			cg->mov(ee_src1_value, cg->dword[cg->rbp + src1_offset]);
			cg->mov(ee_src2_value, cg->dword[cg->rbp + src2_offset]);
			cg->add(ee_src1_value, ee_src2_value);
			cg->mov(cg->dword[cg->rbp + dest_offset], ee_src1_value);
		}
	}
	else
	{
		printf("Unknown argument type for add\n");
		exit(1);
	}
}

void EE_JIT::Emitter::EmitMemoryStore(IRInstruction i)
{
	if (i.args.size() != 3 || !i.args[1].IsImm() || !i.args[0].IsReg() || !i.args[2].IsReg())
	{
		printf("[emu/IRTranslator]: Invalid argument count %ld for MemoryStore\n", i.args.size());
		exit(1);
	}

	reg_alloc->MarkRegUsed(RegisterAllocator::RSI);
	reg_alloc->MarkRegUsed(RegisterAllocator::RDI);

	Xbyak::Reg32 ee_base_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
	auto base_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[2].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
	auto val_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u64)));
	Xbyak::Reg64 func_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());

	cg->mov(ee_base_value, cg->qword[cg->rbp + base_offset]);
	cg->add(ee_base_value, i.args[1].GetImm());
	cg->mov(cg->rdi, ee_base_value);
	cg->mov(cg->rsi, cg->dword[cg->rbp + val_offset]);

	if (i.access_size == IRInstruction::U32)
	{
		cg->mov(func_ptr, reinterpret_cast<uint64_t>(&Bus::Write32));
	}
	else if (i.access_size == IRInstruction::U64)
	{
		cg->mov(func_ptr, reinterpret_cast<uint64_t>(&Bus::Write64));
	}
	else
	{
		printf("Unknown access size %d\n", i.access_size);
		exit(1);
	}

	cg->call(func_ptr);
}

void EE_JIT::Emitter::EmitShift(IRInstruction i)
{
	if (i.args[2].IsImm())
	{
		Xbyak::Reg32 ee_src_value = Xbyak::Reg32(reg_alloc->AllocHostRegister());
		auto src_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[1].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs) + (i.args[0].GetReg() * sizeof(uint128_t)) + offsetof(uint128_t, u32)));

		cg->mov(ee_src_value, cg->dword[cg->rbp + src_offset]);
		
		assert(i.direction == IRInstruction::Left);

		if (i.is_logical)
		{
			cg->shl(ee_src_value, i.args[2].GetImm());
		}
		else
		{
			cg->sal(ee_src_value, i.args[2].GetImm());
		}

		cg->mov(cg->dword[cg->rbp + dest_offset], ee_src_value);
	}
	else
	{
		printf("[emu/JIT]: Unknown SHIFT argument\n");
		exit(1);
	}
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
}

uint8_t *EE_JIT::Emitter::GetFreeBase()
{
	return free_base;
}
