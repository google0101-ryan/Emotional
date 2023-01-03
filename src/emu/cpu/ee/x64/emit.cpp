#include "emit.h"
#include <sys/mman.h>
#include <emu/cpu/ee/EmotionEngine.h>
#include <fstream>

EE_JIT::Emitter* EE_JIT::emit;

void EE_JIT::Emitter::EmitMov(IRInstruction i)
{
	if (i.args[1].IsImm()) // Lui
	{
		Xbyak::Reg64 reg_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		auto offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[1].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u32)));
		cg->lea(reg_ptr, cg->ptr[cg->rax + offset]);
		cg->mov(cg->dword[reg_ptr], i.args[0].GetImm());
	}
	else if (i.args[1].IsCop0()) // Mfc0
	{
		Xbyak::Reg64 ee_reg_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 cop_reg_ptr = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		Xbyak::Reg64 ee_reg_value = Xbyak::Reg64(reg_alloc->AllocHostRegister());
		auto dest_offset = ((offsetof(EmotionEngine::ProcessorState, regs)) + (i.args[0].GetReg() * sizeof(uint128_t) + offsetof(uint128_t, u32)));
		auto src_offset = ((offsetof(EmotionEngine::ProcessorState, cop0_regs)) + (i.args[1].GetReg() * sizeof(uint32_t)));
		cg->lea(cop_reg_ptr, cg->ptr[cg->rax + src_offset]);
		cg->mov(ee_reg_value, cg->dword[cop_reg_ptr]);
		cg->lea(ee_reg_ptr, cg->ptr[cg->rax + dest_offset]);
		cg->mov(cg->dword[ee_reg_ptr], ee_reg_value);
	}
	else
	{
		printf("[emu/JIT]: Unknown MOV format\n");
		exit(1);
	}
}

void EE_JIT::Emitter::EmitSaveHostRegs()
{
	for (int i = 0; i < 8; i++)
		if (i != 4)
			cg->push(Xbyak::Reg64(i));
		
	cg->mov(cg->rbp, reinterpret_cast<uint64_t>(EmotionEngine::GetState()));
}

void EE_JIT::Emitter::EmitIR(IRInstruction i)
{
	reg_alloc->Reset();

	switch (i.instr)
	{
	case SaveHostRegs:
		EmitSaveHostRegs();
		break;
	case MOV:
		EmitMov(i);
		break;
	default:
		printf("[JIT/Emit]: Unknown IR instruction 0x%02x\n", i.instr);
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
