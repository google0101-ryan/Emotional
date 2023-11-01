#include "EEJit.h"
#include <emu/cpu/ee/EmotionEngine.h>
#include <emu/memory/Bus.h>

#if (EE_JIT == 64)
#include <emu/cpu/ee/x64/EEJitx64.h>
#endif

#include <emu/cpu/iop/opcode.h>

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

Block *curBlock;

std::unordered_map<uint32_t, Block*> blocks;

void EmitPrologue()
{
    IRInstruction instr = IRInstruction::Build({}, IRInstrs::PROLOGUE);
    curBlock->instructions.push_back(instr);
}

// 0x05
void EmitBEQ(Opcode op)
{
    IRValue imm(IRValue::Imm);
    imm.SetImm32(op.i_type.imm << 2);

    IRValue rt(IRValue::Reg);
    rt.SetReg(op.r_type.rt);

    IRValue rs(IRValue::Reg);
    rs.SetReg(op.r_type.rs);

    auto instr = IRInstruction::Build({rs, rt, imm}, IRInstrs::BRANCH);
    instr.b_type = IRInstruction::BranchType::EQ;
    curBlock->instructions.push_back(instr);

    printf("beq %s,%s,pc+%d\n", EmotionEngine::Reg(op.i_type.rs), EmotionEngine::Reg(op.i_type.rt), (int32_t)imm.GetImm());
}

// 0x0A
void EmitSLTI(Opcode op)
{
    IRValue imm(IRValue::Imm);
    imm.SetImm64(op.i_type.imm);

    IRValue src(IRValue::Reg);
    src.SetReg(op.i_type.rs);

    IRValue dst(IRValue::Reg);
    dst.SetReg(op.i_type.rt);

    auto instr = IRInstruction::Build({dst, src, imm}, IRInstrs::SLT);
    instr.is_unsigned = false;
    curBlock->instructions.push_back(instr);

    printf("slti %s,%s,0x%08lx\n", EmotionEngine::Reg(op.i_type.rt), EmotionEngine::Reg(op.i_type.rs), (int64_t)(int16_t)op.i_type.imm);
}

// 0x0F
void EmitLUI(Opcode op)
{
    IRValue rt(IRValue::Reg);
    rt.SetReg(op.i_type.rt);

    IRValue imm(IRValue::Imm);
    imm.SetImm64From32(op.i_type.imm << 16);

    auto instr = IRInstruction::Build({rt, imm}, IRInstrs::MOVE);
    curBlock->instructions.push_back(instr);

    printf("lui %s,0x%08lx\n", EmotionEngine::Reg(rt.GetReg()), imm.GetImm64());
}

// 0x10 0x00
void EmitMFC0(Opcode op)
{
    int dest = op.r_type.rt;
    int src = op.r_type.rd;

    IRValue dst_val(IRValue::Cop0Reg);
    dst_val.SetReg(dest);

    IRValue src_val(IRValue::Reg);
    src_val.SetReg(src);

    auto instr = IRInstruction::Build({src_val, dst_val}, IRInstrs::MOVE);
    curBlock->instructions.push_back(instr);

    printf("mfc0 %s,r%d\n", EmotionEngine::Reg(dest), src);
}

// 0x10
void EmitCOP0(Opcode op)
{
    switch (op.r_type.func)
    {
    case 0x00:
        EmitMFC0(op);
        break;
    default:
        printf("[EEJIT]: Cannot emit unknown cop0 opcode 0x%08x\n", op.r_type.func);
        exit(1);
    }
}

bool IsBranch(Opcode op)
{
    switch (op.opcode)
    {
    case 0x05:
        return true;
    }

    return false;
}

bool branchDelayed = false;

// Compile and dispatch `cycles` instructions
// If we encounter a branch, return that as the true number of cycles
int EEJit::Clock(int cycles)
{
    // Create a new block
    curBlock = new Block();
    curBlock->addr = EmotionEngine::GetState()->pc;
    curBlock->instructions.clear();

    uint32_t start = curBlock->addr;

    EmitPrologue();

    int cycle = 0;
    for (; cycle < cycles; cycle++)
    {
        printf("0x%08x:\t", start);
        // TODO: Move this into its own assembly routine
        uint32_t instr = Bus::Read32(start);
        start += 4;

        Opcode op;
        op.full = instr;

        if (!instr)
        {
            printf("nop\n");
            if (branchDelayed)
            {
                branchDelayed = false;
                cycle++;
                break;
            }
            continue;
        }

        switch (op.opcode)
        {
        case 0x05:
            EmitBEQ(op);
            break;
        case 0x0A:
            EmitSLTI(op);
            break;
        case 0x0F:
            EmitLUI(op);
            break;
        case 0x10:
            EmitCOP0(op);
            break;
        default:
            printf("[EEJIT]: Cannot emit unknown opcode 0x%02x (0x%08x)\n", op.opcode, op.full);
            exit(1);
        }
        
        if (branchDelayed)
        {
            branchDelayed = false;
            cycle++;
            break;
        }

        branchDelayed = IsBranch(op);
    }

    // Emit epilogue
    curBlock->instructions.push_back(IRInstruction::Build({}, IRInstrs::EPILOGUE));
    // JIT the block into host code
#if EE_JIT == 64
    EEJitX64::TranslateBlock(curBlock);
#endif
    // Cache the block
    // Run it
    curBlock->entryPoint(EmotionEngine::GetState(), start);

    return cycle;
}

void EEJit::Initialize()
{
#if EE_JIT == 64
    EEJitX64::Initialize();
#else
#error Please use x64! x86/AARCH64 currently unsupported!
#endif
}

void EEJit::Dump()
{
#if EE_JIT == 64
    EEJitX64::Dump();
#endif
}
