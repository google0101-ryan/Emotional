#include "EEJitx64.h"
#include "RegAllocator.h"
#include <emu/cpu/ee/EEJit.h>
#include <emu/cpu/ee/EmotionEngine.h>
#include <emu/memory/Bus.h>

#include <cstdio>
#include <cstdlib>
#include <3rdparty/xbyak/xbyak.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <unordered_map>

std::unordered_map<uint32_t, Block*> blockMap;

Xbyak::CodeGenerator* generator;
uint8_t* base;
RegAllocatorX64 reg_alloc;

#include "EEJitX64_Aliases.inl"

void EEJitX64::JitStoreReg(GuestRegister reg)
{
    auto offs = reg_alloc.GetRegOffset(reg);

    if (reg >= COP0_OFFS && reg < COP0_OFFS+32)
        MOV(generator->dword[generator->rbp + offs], Xbyak::Reg32(reg_alloc.GetHostReg(reg)));
    else
        MOV(generator->qword[generator->rbp + offs], Xbyak::Reg64(reg_alloc.GetHostReg(reg)));
}

void EEJitX64::JitLoadReg(GuestRegister reg, int hostReg)
{
    if (reg >= COP0_OFFS && reg < COP0_OFFS+32)
        MOV(Xbyak::Reg32(hostReg), generator->dword[generator->rbp + reg_alloc.GetRegOffset(reg)]);
    else
        MOV(Xbyak::Reg64(hostReg), generator->qword[generator->rbp + reg_alloc.GetRegOffset(reg)]);
}

void SaveHostRegisters()
{
    generator->push(generator->rbx);
    generator->push(generator->rdx);
    generator->push(generator->r8);
    generator->push(generator->r9);
    generator->push(generator->r10);
    generator->push(generator->r11);
}

void RestoreHostRegisters()
{
    generator->pop(generator->r11);
    generator->pop(generator->r10);
    generator->pop(generator->r9);
    generator->pop(generator->r8);
    generator->pop(generator->rdx);
    generator->pop(generator->rbx);
}

void JitPrologue()
{
    // Load RBP with the state pointer, passed in RDI
    for (int i = 0; i < 16; i++)
        if (i != 4)
            generator->push(Xbyak::Reg64(i));
    MOV(generator->rbp, generator->rdi);
    MOV(generator->r8, generator->rsi);
}

void JitEpilogue()
{
    // First, we need to writeback all registers to memory
    reg_alloc.DoWriteback();

    // Write R8 back to pc
    MOV(generator->qword[generator->rbp + offsetof(EmotionEngine::ProcessorState, pc)], generator->r8);
    MOV(generator->qword[generator->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)], generator->r8);
    ADD(generator->qword[generator->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)], 4);

    // Now restore all host registers
    for (int i = 15; i >= 0; i--)
        if (i != 4)
            generator->pop(Xbyak::Reg64(i));
    generator->ret();
}

void JitMov(IRInstruction& i)
{
    if (i.args[0].IsReg() && i.args[1].IsCop0())
    {
        if (i.args[1].GetReg() == 0)
        {
            printf("WARNING: Mov cop0 -> $zero\n");
            return;
        }
        else
        {
            auto src = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)(i.args[1].GetReg()+COP0_OFFS)));
            auto dst = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)i.args[0].GetReg(), true));
            MOV(dst, src);
        }
    }
    else if (i.args[1].IsReg() && i.args[0].IsCop0())
    {
        auto src = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
        auto dst = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)(i.args[0].GetReg()+COP0_OFFS), true));
        MOV(dst, src);
    }
    else if (i.args[0].IsReg() && i.args[1].IsImm())
    {
        if (i.args[1].GetReg() == 0)
        {
            printf("WARNING: Mov imm -> $zero\n");
            return;
        }
        else
        {
            auto dst = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)(i.args[0].GetReg()), true));
            MOV(dst, i.args[1].GetImm64());
        }
    }
    else if (i.args[0].IsReg() && i.args[1].IsSpecial())
    {
        if (i.args[0].GetReg() == 0)
        {
            printf("WARNING: Mov imm -> $zero\n");
            return;
        }
        else
        {
            auto dst = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)(i.args[0].GetReg()), true));
            auto src = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)(i.args[1].GetReg())));
			MOV(dst, src);
        }
    }
    else
    {
        printf("[EEJIT_X64]: Unknown src/dst combo for move\n");
        exit(1);
    }
}

void JitSlt(IRInstruction& i)
{
    // Args[0] = dst, Args[1] = op1, Args[2] = op2
    if (i.args[2].IsImm())
    {
        auto dst = Xbyak::Reg8(reg_alloc.GetHostReg((GuestRegister)i.args[0].GetReg(), true));
        if (i.args[1].GetReg() == 0)
        {
            int32_t imm = i.args[2].GetImm();
            MOV(generator->rdi, 0);
            generator->cmp(generator->rdi, imm);
			if (i.is_unsigned)
				generator->seta(dst);
			else
	        	generator->setl(dst);
            generator->movzx(Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[0].GetReg(), true)), dst);
        }
        else
        {
            auto src = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
            int32_t imm = i.args[2].GetImm();

            generator->cmp(src, imm);
			if (i.is_unsigned)
				generator->seta(dst);
			else
	        	generator->setl(dst);
            generator->movzx(Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[0].GetReg(), true)), dst);
        }
    }
    else
    {
        printf("Unhandled SLT/SLTU\n");
        exit(1);
    }
}

void JitBranch(IRInstruction& i)
{
    auto op1 = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[0].GetReg()));

    Xbyak::Label cond_failed;

	switch (i.b_type)
    {
    case IRInstruction::BranchType::EQ:
    {
        if (i.args[1].GetReg() == 0)
        {
            generator->cmp(op1, 0);
            generator->jne(cond_failed);
        }
        else if (i.args[0].GetReg() == 0)
        {
            auto op2 = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
            generator->cmp(op2, 0);
            generator->jne(cond_failed);
        }
        else
        {
            auto op2 = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
            generator->cmp(op1, op2);
            generator->jne(cond_failed);
        }
        break;
    }
    case IRInstruction::BranchType::NE:
    {
        if (i.args[1].GetReg() == 0)
        {
            generator->cmp(op1, 0);
            generator->je(cond_failed);
        }
        else if (i.args[0].GetReg() == 0)
        {
            auto op2 = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
            MOV(generator->rdi, 0);
            generator->cmp(generator->rdi, i.args[1].GetImm());
            generator->je(cond_failed);
        }
        else
        {
            auto op2 = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
            generator->cmp(op1, op2);
            generator->je(cond_failed);
        }
        break;
    }
    case IRInstruction::BranchType::AL:
        break;
    default:
        printf("Unknown branch condition %d\n", i.b_type);
        exit(1);
    }

    Xbyak::Label done;

    ADD(generator->r8, i.args[2].GetImm()-4);
    generator->jmp(done);
    generator->L(cond_failed);
    ADD(generator->r8, 4);
	if (i.is_likely)
	{
	    ADD(generator->r8, 4);
		JitEpilogue();
	}
    generator->L(done);
}

void JitOr(IRInstruction& i)
{
    if (i.args[2].IsImm())
    {
        if (i.args[1].GetReg() == i.args[0].GetReg())
        {
            auto src = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
            generator->or_(src, i.args[2].GetImm());
            return;
        }
        auto src = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
        auto dst = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[0].GetReg(), true));

        MOV(generator->rdi, src);
        generator->or_(generator->rdi, i.args[2].GetImm());
        MOV(dst, generator->rdi);
    }
    else
    {
		if (i.args[1].GetReg() == 0)
		{
			auto src = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[2].GetReg()));
	        auto dst = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[0].GetReg(), true));
			MOV(dst, src);
		}
		else if (i.args[2].GetReg() == 0)
		{
			auto src = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
	        auto dst = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[0].GetReg(), true));
			MOV(dst, src);
		}
		else
		{
			auto src1 = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
	        auto src2 = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[2].GetReg()));
	        auto dst = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[0].GetReg(), true));
			generator->or_(src1, src2);
			MOV(dst, src1);
		}
    }
}

void JitJump(IRInstruction& i)
{
    if (i.args[0].IsReg() && i.args.size() == 1)
    {
        if (i.should_link)
        {
            auto lr = Xbyak::Reg64(reg_alloc.GetHostReg(REG_RA, true));
            MOV(lr, generator->r8);
        }

        auto src = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[0].GetReg()));
        MOV(generator->r8, src);
    }
	else if (i.args[0].IsReg() && i.args.size() == 2)
    {
		assert(i.should_link);
       	auto lr = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[0].GetReg(), true));
        MOV(lr, generator->r8);
    
        auto src = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
        generator->mov(generator->r8d, src);
    }
    else
    {
		if (i.should_link)
		{
			auto lr = Xbyak::Reg64(reg_alloc.GetHostReg(REG_RA, true));
            MOV(lr, generator->r8);
		}

		generator->and_(generator->r8, 0xF0000000);
		generator->or_(generator->r8, i.args[0].GetImm());
    }
}

void JitAdd(IRInstruction instr)
{
    if (instr.args[1].IsReg() && instr.args[2].IsImm() && instr.size == IRInstruction::Size32)
    {
        if (instr.args[1].GetReg() == 0)
        {
            auto dst = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)instr.args[0].GetReg(), true));
            auto imm = instr.args[2].GetImm();
            MOV(dst, imm);
        }
        else
        {
            auto src = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)instr.args[1].GetReg()));
            auto dst = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)instr.args[0].GetReg(), true));
            auto imm = instr.args[2].GetImm();

            MOV(generator->rdi, src);
            ADD(generator->rdi, imm);
            MOV(dst, generator->rdi);
        }
    }
    else if (instr.args[1].IsReg() && instr.args[2].IsReg() && instr.size == IRInstruction::Size64)
    {
		if (instr.args[1].GetReg() == 0 && instr.args[2].GetReg() == 0)
		{
			auto dst = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)instr.args[0].GetReg(), true));
            MOV(dst, 0);
		}
        else if (instr.args[1].GetReg() == 0)
        {
            auto dst = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)instr.args[0].GetReg(), true));
            auto imm = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)instr.args[2].GetReg()));
            MOV(dst, imm);
        }
		else if (instr.args[2].GetReg() == 0)
        {
            auto dst = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)instr.args[0].GetReg(), true));
            auto imm = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)instr.args[1].GetReg()));
            MOV(dst, imm);
        }
        else
        {
            auto src = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)instr.args[1].GetReg()));
            auto dst = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)instr.args[0].GetReg(), true));
            auto src2 = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)instr.args[2].GetReg()));

            MOV(generator->rdi, src);
            ADD(generator->rdi, src2);
            MOV(dst, generator->rdi);
        }
    }
    else
    {
		printf("%d, %d\n", instr.args[1].type, instr.args[2].type);
        assert(0 && "Unknown add combo");
    }
}

void JitStore(IRInstruction instr)
{
    if (instr.access_size == IRInstruction::U32)
    {
        auto src = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)instr.args[2].GetReg()));
        auto dst = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)instr.args[0].GetReg()));
        auto imm = instr.args[1].GetImm();

        SaveHostRegisters();

        MOV(generator->rdi, src);
        ADD(generator->rdi, imm);
        MOV(generator->rsi, dst);
        MOV(generator->rcx, reinterpret_cast<uint64_t>(Bus::Write32));
        generator->call(generator->rcx);

        RestoreHostRegisters();
    }
    else if (instr.access_size == IRInstruction::U64)
    {
        auto src = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)instr.args[2].GetReg()));
        auto dst = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)instr.args[0].GetReg()));
        auto imm = instr.args[1].GetImm();

        SaveHostRegisters();

        MOV(generator->rdi, src);
        ADD(generator->rdi, imm);
        MOV(generator->rsi, dst);
        MOV(generator->rcx, reinterpret_cast<uint64_t>(Bus::Write64));
        generator->call(generator->rcx);

        RestoreHostRegisters();
    }
    else
    {
        printf("Unknown store access size %d\n", (int)instr.access_size);
        assert(0);
    }
}

void JitAnd(IRInstruction& i)
{
    if (i.args[2].IsImm())
    {
        if (i.args[1].GetReg() == i.args[0].GetReg())
        {
            auto src = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
            generator->and_(src, i.args[2].GetImm());
            return;    
        }
        auto src = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
        auto dst = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[0].GetReg(), true));

        MOV(generator->edi, src);
        generator->and_(generator->edi, i.args[2].GetImm());
        generator->movsxd(dst, generator->edi);
    }
    else
    {
        printf("TODO: and\n");
        exit(1);
    }
}

void JitShift(IRInstruction i)
{
	// SLL
	if (i.args[2].IsImm() && i.is_logical && i.direction == IRInstruction::Direction::Left)
	{
		auto src = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
        auto dst = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)i.args[0].GetReg(), true));

		MOV(generator->cl, i.args[2].GetImm());
		generator->shl(src, generator->cl);
		generator->mov(dst, src);
	}
	else if (i.args[2].IsImm() && i.is_logical && i.direction == IRInstruction::Direction::Right)
	{
		auto src = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
        auto dst = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)i.args[0].GetReg(), true));

		MOV(generator->cl, i.args[2].GetImm());
		generator->shr(src, generator->cl);
		generator->mov(dst, src);
	}
	else
	{
		printf("Invalid shift combo %d/%d/%d\n", i.args[2].type, i.is_logical, i.direction);
		exit(1);
	}
}

void JitMULT(IRInstruction i)
{
	GuestRegister lo_reg = i.is_mmi_divmul ? GuestRegister::LO1 : GuestRegister::LO;
	GuestRegister hi_reg = i.is_mmi_divmul ? GuestRegister::HI1 : GuestRegister::HI;

	auto src = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
	auto src2 = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)i.args[2].GetReg()));
	auto lo = Xbyak::Reg64(reg_alloc.GetHostReg(lo_reg));
	auto hi = Xbyak::Reg64(reg_alloc.GetHostReg(hi_reg));

	reg_alloc.InvalidateRegister(HostRegisters::RDX);

	if (i.is_unsigned)
	{
		generator->xor_(generator->rdx, generator->rdx);
		generator->movsxd(generator->rax, src);
		generator->mul(src2);
		generator->movsxd(lo, generator->eax);
		generator->movsxd(hi, generator->edx);

		if (i.args[0].GetReg())
		{
			auto dst = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)i.args[0].GetReg()));
			MOV(dst, lo);
		}
	}
	else
	{
		printf("TODO: MULT");
		assert(0);
	}
}

void JitDIV(IRInstruction i)
{
	GuestRegister lo_reg = i.is_mmi_divmul ? GuestRegister::LO1 : GuestRegister::LO;
	GuestRegister hi_reg = i.is_mmi_divmul ? GuestRegister::HI1 : GuestRegister::HI;

	auto src = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
	auto src2 = Xbyak::Reg32(reg_alloc.GetHostReg((GuestRegister)i.args[2].GetReg()));
	auto lo = Xbyak::Reg64(reg_alloc.GetHostReg(lo_reg));
	auto hi = Xbyak::Reg64(reg_alloc.GetHostReg(hi_reg));

	reg_alloc.InvalidateRegister(HostRegisters::RDX);

	if (i.is_unsigned)
	{
		generator->xor_(generator->rdx, generator->rdx);
		MOV(generator->rax, src);
		generator->div(src2);
		MOV(lo, generator->rax);
		MOV(hi, generator->rdx);
	}
	else
	{
		printf("TODO: DIV");
		assert(0);
	}
}

void JitIncPC()
{
    ADD(generator->r8, 4);
}

void EEJitX64::TranslateBlock(Block *block)
{
	printf("Translating block at 0x%08x\n", block->addr);
    reg_alloc.Reset();

    block->entryPoint = (blockEntry)generator->getCurr();

    IRInstruction prev;

    for (auto& i : block->instructions)
    {
        if (i.instr != PROLOGUE && i.instr != EPILOGUE && i.instr != BRANCH && prev.instr != JUMP)
            JitIncPC();
        
        switch (i.instr)
        {
        case NOP:
            generator->nop();
            break;
        case PROLOGUE:
            JitPrologue();
            break;
        case EPILOGUE:
            JitEpilogue();
            break;
        case MOVE:
            JitMov(i);
            break;
        case SLT:
            JitSlt(i);
            break;
        case BRANCH:
            JitBranch(i);
            break;
        case OR:
            JitOr(i);
            break;
        case JUMP:
            JitJump(i);
            break;
        case ADD:
            JitAdd(i);
            break;
        case STORE:
            JitStore(i);
            break;
		case AND:
			JitAnd(i);
			break;
		case SHIFT:
			JitShift(i);
			break;
		case MULT:
			JitMULT(i);
			break;
		case DIV:
			JitDIV(i);
			break;
		case BREAK:
			generator->ud2();
			break;
        default:
            printf("[EEJIT_X64]: Cannot emit unknown IR instruction %d\n", i.instr);
            exit(1);
        }

        prev = i;
    }
}

void EEJitX64::CacheBlock(Block *block)
{
    // TODO: Actually cache the block
	blockMap[block->addr] = block;

    if ((generator->getCurr() - generator->getCode()) >= (128*1024*1024))
    {
        delete[] generator;
        generator = new Xbyak::CodeGenerator(0xffffffff, (void*)base);
		blockMap.clear();
    }
}

Block *EEJitX64::GetBlockForAddr(uint32_t addr)
{
	return blockMap[addr];
}

void EEJitX64::Initialize()
{
    base = (uint8_t*)mmap(nullptr, 0xffffffff, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (base == MAP_FAILED)
	{
		printf("[EEJIT_X64]: Failed to map base! %s\n", strerror(errno));
		exit(1);
	}

    generator = new Xbyak::CodeGenerator(0xffffffff, (void*)base);
}

void EEJitX64::Dump()
{
    std::ofstream file("code.out", std::ios::binary);
    file.write((char*)base, generator->getSize());
    file.close();
}
