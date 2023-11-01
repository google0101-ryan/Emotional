#include "EEJitx64.h"
#include "RegAllocator.h"
#include <emu/cpu/ee/EEJit.h>
#include <emu/cpu/ee/EmotionEngine.h>

#include <cstdio>
#include <cstdlib>
#include <3rdparty/xbyak/xbyak.h>
#include <cerrno>
#include <cstring>
#include <fstream>

Xbyak::CodeGenerator* generator;
uint8_t* base;
RegAllocatorX64 reg_alloc;

#include "EEJitX64_Aliases.inl"

void EEJitX64::JitStoreReg(GuestRegister reg)
{
    auto offs = reg_alloc.GetRegOffset(reg);

    generator->mov(generator->qword[generator->rbp + offs], Xbyak::Reg64(reg_alloc.GetHostReg(reg)));
}

void EEJitX64::JitLoadReg(GuestRegister reg, int hostReg)
{
    generator->mov(Xbyak::Reg64(hostReg), generator->qword[generator->rbp + reg_alloc.GetRegOffset(reg)]);
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
    generator->mov(generator->qword[generator->rbp + offsetof(EmotionEngine::ProcessorState, pc)], generator->r8);
    generator->mov(generator->qword[generator->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)], generator->r8);
    generator->add(generator->qword[generator->rbp + offsetof(EmotionEngine::ProcessorState, next_pc)], 4);

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
            auto src = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)(i.args[0].GetReg()+COP0_OFFS)));
            auto dst = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg(), true));
            generator->mov(dst, src);
        }
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
            generator->mov(dst, i.args[1].GetImm64());
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
        if (i.is_unsigned)
        {
            printf("TODO: SLTIU\n");
            exit(1);
        }
        else
        {
            auto dst = Xbyak::Reg8(reg_alloc.GetHostReg((GuestRegister)i.args[0].GetReg(), true));
            auto src = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
            int32_t imm = i.args[2].GetImm();

            generator->cmp(src, imm);
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
        auto op2 = Xbyak::Reg64(reg_alloc.GetHostReg((GuestRegister)i.args[1].GetReg()));
        generator->cmp(op1, op2);
        generator->jne(cond_failed);
        break;
    }
    default:
        printf("Unknown branch condition %d\n", i.b_type);
        exit(1);
    }

    generator->add(generator->r8, i.args[2].GetImm());
    generator->L(cond_failed);
}

void JitIncPC()
{
    generator->add(generator->r8, 4);
}

void EEJitX64::TranslateBlock(Block *block)
{
    block->entryPoint = (blockEntry)generator->getCurr();

    for (auto& i : block->instructions)
    {
        if (i.instr != 0x00)
            JitIncPC();

        switch (i.instr)
        {
        case 0x00:
            JitPrologue();
            break;
        case 0x01:
            JitEpilogue();
            break;
        case 0x02:
            JitMov(i);
            break;
        case 0x03:
            JitSlt(i);
            break;
        case 0x04:
            JitBranch(i);
            break;
        default:
            printf("[EEJIT_X64]: Cannot emit unknown IR instruction 0x%02x\n", i.instr);
            exit(1);
        }
    }
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
