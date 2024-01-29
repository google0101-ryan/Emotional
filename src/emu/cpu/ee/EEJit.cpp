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

void EmitSLL(Opcode op)
{
	IRValue rd(IRValue::Reg);
	rd.SetReg(op.r_type.rd);
	
	IRValue rt(IRValue::Reg);
	rt.SetReg(op.r_type.rt);
	
	IRValue sa(IRValue::Imm);
	sa.SetImm32Unsigned(op.r_type.sa);

	IRInstruction instr = IRInstruction::Build({rd, rt, sa}, SHIFT);
	instr.is_logical = true;
	instr.direction = IRInstruction::Direction::Left;
	curBlock->instructions.push_back(instr);

	printf("sll %s,%s,%d\n", EmotionEngine::Reg(op.r_type.rd), EmotionEngine::Reg(op.r_type.rt), op.r_type.sa);
}

// 0x02
void EmitSRL(Opcode op)
{
	IRValue rd(IRValue::Reg);
	rd.SetReg(op.r_type.rd);
	
	IRValue rt(IRValue::Reg);
	rt.SetReg(op.r_type.rt);
	
	IRValue sa(IRValue::Imm);
	sa.SetImm32Unsigned(op.r_type.sa);

	IRInstruction instr = IRInstruction::Build({rd, rt, sa}, SHIFT);
	instr.is_logical = true;
	instr.direction = IRInstruction::Direction::Right;
	curBlock->instructions.push_back(instr);

	printf("srl %s,%s,%d\n", EmotionEngine::Reg(op.r_type.rd), EmotionEngine::Reg(op.r_type.rt), op.r_type.sa);
}

void EmitJR(Opcode op)
{
    IRValue reg(IRValue::Reg);
    reg.SetReg(op.r_type.rs);

    auto instr = IRInstruction::Build({reg}, JUMP);
    instr.should_link = false;
    curBlock->instructions.push_back(instr);

    printf("jr %s\n", EmotionEngine::Reg(reg.GetReg()));
}

// 0x09
void EmitJalr(Opcode op)
{
	IRValue rd(IRValue::Reg);
	rd.SetReg(op.r_type.rd);
	IRValue rs(IRValue::Reg);
	rs.SetReg(op.r_type.rs);

	auto instr = IRInstruction::Build({rd, rs}, JUMP);
	instr.should_link = true;
	curBlock->instructions.push_back(instr);

	printf("jalr %s,%s\n", EmotionEngine::Reg(op.r_type.rd), EmotionEngine::Reg(op.r_type.rs));
}

// 0x0d
void EmitBreak()
{
	auto instr = IRInstruction::Build({}, BREAK);
	curBlock->instructions.push_back(instr);

	printf("break\n");
}

void EmitMFLO(Opcode op)
{
	IRValue rd(IRValue::Reg);
	rd.SetReg(op.r_type.rd);

	IRValue lo(IRValue::Special);
	lo.SetReg(LO);

	IRInstruction instr = IRInstruction::Build({rd, lo}, MOVE);
	instr.is_mmi_divmul = false;
	curBlock->instructions.push_back(instr);

	printf("mflo %s\n", EmotionEngine::Reg(op.r_type.rd));
}

// 0x18
void EmitMULT(Opcode op)
{
	IRValue rd(IRValue::Reg);
	rd.SetReg(op.r_type.rd);
	IRValue rt(IRValue::Reg);
	rt.SetReg(op.r_type.rt);
	IRValue rs(IRValue::Reg);
	rs.SetReg(op.r_type.rs);

	auto instr = IRInstruction::Build({rd, rs, rt}, MULT);
	instr.is_unsigned = true;
	instr.size = IRInstruction::InstrSize::Size32;
	instr.is_mmi_divmul = false;
	curBlock->instructions.push_back(instr);

	printf("mult %s,%s,%s\n", EmotionEngine::Reg(op.r_type.rd), EmotionEngine::Reg(op.r_type.rs), EmotionEngine::Reg(op.r_type.rt));
}

// 0x1B
void EmitDIVU(Opcode op)
{
	IRValue rd(IRValue::Reg);
	rd.SetReg(op.r_type.rt);
	IRValue rt(IRValue::Reg);
	rt.SetReg(op.r_type.rt);
	IRValue rs(IRValue::Reg);
	rs.SetReg(op.r_type.rs);

	auto instr = IRInstruction::Build({rd, rs, rt}, DIV);
	instr.is_unsigned = true;
	instr.size = IRInstruction::InstrSize::Size32;
	instr.is_mmi_divmul = false;
	curBlock->instructions.push_back(instr);

	printf("divu %s,%s,%s\n", EmotionEngine::Reg(op.r_type.rd), EmotionEngine::Reg(op.r_type.rs), EmotionEngine::Reg(op.r_type.rt));
}

// 0x25
void EmitOR(Opcode op)
{
	IRValue rt(IRValue::Reg);
	rt.SetReg(op.r_type.rt);
	IRValue rd(IRValue::Reg);
	rd.SetReg(op.r_type.rd);
	IRValue rs(IRValue::Reg);
	rs.SetReg(op.r_type.rs);

	auto instr = IRInstruction::Build({rd, rs, rt}, OR);
	instr.is_unsigned = true;
	instr.size = IRInstruction::InstrSize::Size64;

	curBlock->instructions.push_back(instr);

	printf("or %s,%s,%s\n", EmotionEngine::Reg(rd.GetReg()), EmotionEngine::Reg(rs.GetReg()), EmotionEngine::Reg(rt.GetReg()));
}

// 0x2d
void EmitDADDU(Opcode op)
{
	IRValue rt(IRValue::Reg);
	rt.SetReg(op.r_type.rt);
	IRValue rd(IRValue::Reg);
	rd.SetReg(op.r_type.rd);
	IRValue rs(IRValue::Reg);
	rs.SetReg(op.r_type.rs);

	auto instr = IRInstruction::Build({rd, rs, rt}, ADD);
	instr.is_unsigned = true;
	instr.size = IRInstruction::InstrSize::Size64;

	curBlock->instructions.push_back(instr);

	printf("daddu %s,%s,%s\n", EmotionEngine::Reg(rd.GetReg()), EmotionEngine::Reg(rs.GetReg()), EmotionEngine::Reg(rt.GetReg()));
}

// 0x00
void EmitSpecial(Opcode op)
{
    switch (op.r_type.func)
    {
	case 0x00:
		EmitSLL(op);
		break;
	case 0x02:
		EmitSRL(op);
		break;
    case 0x08:
        EmitJR(op);
        break;
	case 0x09:
		EmitJalr(op);
		break;
	case 0x0d:
		EmitBreak();
		break;
    case 0x0f:
        printf("sync\n");
        break;
	case 0x12:
		EmitMFLO(op);
		break;
	case 0x18:
		EmitMULT(op);
		break;
	case 0x1b:
		EmitDIVU(op);
		break;
	case 0x25:
		EmitOR(op);
		break;
	case 0x2D:
	    EmitDADDU(op);
		break;
    default:
        printf("[EEJIT]: Cannot emit unknown special opcode 0x%02x (0x%08x)\n", op.r_type.func, op.full);
        exit(1);
    }
}

// 0x03
void EmitJAL(Opcode op)
{
	IRValue imm(IRValue::Imm);
	imm.SetImm32Unsigned(op.j_type.target << 2);

	auto instr = IRInstruction::Build({imm}, JUMP);
	instr.should_link = true;
	curBlock->instructions.push_back(instr);

	printf("jal 0x%08x\n", (EmotionEngine::GetState()->pc & 0xF0000000) | imm.GetImm());
}

// 0x04
void EmitBEQ(Opcode op)
{
    IRValue imm(IRValue::Imm);
    imm.SetImm32(op.i_type.imm << 2);

    IRValue rt(IRValue::Reg);
    rt.SetReg(op.r_type.rt);

    IRValue rs(IRValue::Reg);
    rs.SetReg(op.r_type.rs);

    auto instr = IRInstruction::Build({rs, rt, imm}, IRInstrs::BRANCH);
    if (op.r_type.rt == op.r_type.rs == 0)
        instr.b_type = IRInstruction::BranchType::AL;
    else
        instr.b_type = IRInstruction::BranchType::EQ;
    curBlock->instructions.push_back(instr);

    printf("beq %s,%s,pc+%d\n", EmotionEngine::Reg(op.i_type.rs), EmotionEngine::Reg(op.i_type.rt), (int32_t)imm.GetImm());
}

// 0x05
void EmitBNE(Opcode op)
{
    IRValue imm(IRValue::Imm);
    imm.SetImm32(op.i_type.imm << 2);

    IRValue rt(IRValue::Reg);
    rt.SetReg(op.r_type.rt);

    IRValue rs(IRValue::Reg);
    rs.SetReg(op.r_type.rs);

    auto instr = IRInstruction::Build({rs, rt, imm}, IRInstrs::BRANCH);
    instr.b_type = IRInstruction::BranchType::NE;
    curBlock->instructions.push_back(instr);

    printf("bne %s,%s,pc+%d\n", EmotionEngine::Reg(op.i_type.rs), EmotionEngine::Reg(op.i_type.rt), (int32_t)imm.GetImm());
}

// 0x09
void EmitADDIU(Opcode op)
{
    IRValue imm(IRValue::Imm);
    imm.SetImm64(op.i_type.imm);

    IRValue src(IRValue::Reg);
    src.SetReg(op.i_type.rs);

    IRValue dst(IRValue::Reg);
    dst.SetReg(op.i_type.rt);

    auto instr = IRInstruction::Build({dst, src, imm}, IRInstrs::ADD);
    instr.is_unsigned = true;
	instr.size = IRInstruction::Size32;
    curBlock->instructions.push_back(instr);

    printf("addiu %s,%s,0x%08lx\n", EmotionEngine::Reg(op.i_type.rt), EmotionEngine::Reg(op.i_type.rs), (int64_t)(int16_t)op.i_type.imm);
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

// 0x0B
void EmitSLTIU(Opcode op)
{
    IRValue imm(IRValue::Imm);
    imm.SetImm64(op.i_type.imm);

    IRValue src(IRValue::Reg);
    src.SetReg(op.i_type.rs);

    IRValue dst(IRValue::Reg);
    dst.SetReg(op.i_type.rt);

    auto instr = IRInstruction::Build({dst, src, imm}, IRInstrs::SLT);
    instr.is_unsigned = true;
    curBlock->instructions.push_back(instr);

    printf("sltiu %s,%s,0x%08lx\n", EmotionEngine::Reg(op.i_type.rt), EmotionEngine::Reg(op.i_type.rs), (int64_t)(int16_t)op.i_type.imm);
}

// 0x0C
void EmitANDI(Opcode op)
{
	IRValue imm(IRValue::Imm);
    imm.SetImmUnsigned(op.i_type.imm);

    IRValue src(IRValue::Reg);
    src.SetReg(op.i_type.rs);

    IRValue dst(IRValue::Reg);
    dst.SetReg(op.i_type.rt);

    auto instr = IRInstruction::Build({dst, src, imm}, IRInstrs::AND);
    curBlock->instructions.push_back(instr);

    printf("andi %s,%s,0x%08lx\n", EmotionEngine::Reg(dst.GetReg()), EmotionEngine::Reg(src.GetReg()), imm.GetImm64());
}

// 0x0D
void EmitOri(Opcode op)
{
    IRValue imm(IRValue::Imm);
    imm.SetImmUnsigned(op.i_type.imm);

    IRValue src(IRValue::Reg);
    src.SetReg(op.i_type.rs);

    IRValue dst(IRValue::Reg);
    dst.SetReg(op.i_type.rt);

    auto instr = IRInstruction::Build({dst, src, imm}, IRInstrs::OR);
    curBlock->instructions.push_back(instr);

    printf("ori %s,%s,0x%08lx\n", EmotionEngine::Reg(dst.GetReg()), EmotionEngine::Reg(src.GetReg()), imm.GetImm64());
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
    dst_val.SetReg(src);

    IRValue src_val(IRValue::Reg);
    src_val.SetReg(dest);

    auto instr = IRInstruction::Build({src_val, dst_val}, IRInstrs::MOVE);
    curBlock->instructions.push_back(instr);

    printf("mfc0 %s,r%d\n", EmotionEngine::Reg(dest), src);
}

// 0x10 0x04
void EmitMTC0(Opcode op)
{
    int dest = op.r_type.rt;
    int src = op.r_type.rd;

    IRValue src_val(IRValue::Cop0Reg);
    src_val.SetReg(src);

    IRValue dst_val(IRValue::Reg);
    dst_val.SetReg(dest);

    auto instr = IRInstruction::Build({src_val, dst_val}, IRInstrs::MOVE);
    curBlock->instructions.push_back(instr);

    printf("mtc0 %s,r%d\n", EmotionEngine::Reg(dest), src);
}

// 0x10
void EmitCOP0(Opcode op)
{
    switch (op.r_type.rs)
    {
    case 0x00:
        EmitMFC0(op);
        break;
    case 0x04:
        EmitMTC0(op);
        break;
    case 0x10:
    {
        switch (op.r_type.func)
        {
        case 0x02:
            printf("tlbwi\n");
            break;
        default:
            printf("Unknown COP0 TLB instruction 0x%02x\n", op.r_type.func);
            exit(1);
        }
        break;
    }
    default:
        printf("[EEJIT]: Cannot emit unknown cop0 opcode 0x%08x\n", op.r_type.rs);
        exit(1);
    }
}

// 0x14
void EmitBEQL(Opcode op)
{
    IRValue imm(IRValue::Imm);
    imm.SetImm32(op.i_type.imm << 2);

    IRValue rt(IRValue::Reg);
    rt.SetReg(op.i_type.rt);

    IRValue rs(IRValue::Reg);
    rs.SetReg(op.i_type.rs);

    auto instr = IRInstruction::Build({rs, rt, imm}, IRInstrs::BRANCH);
    if (op.i_type.rt == 0 && op.i_type.rs == 0)
        instr.b_type = IRInstruction::BranchType::AL;
    else
        instr.b_type = IRInstruction::BranchType::EQ;
	instr.is_likely = true;
    curBlock->instructions.push_back(instr);

    printf("beql %s,%s,pc+%d\n", EmotionEngine::Reg(op.i_type.rs), EmotionEngine::Reg(op.i_type.rt), (int32_t)imm.GetImm());
}

// 0x15
void EmitBNEL(Opcode op)
{
    IRValue imm(IRValue::Imm);
    imm.SetImm32(op.i_type.imm << 2);

    IRValue rt(IRValue::Reg);
    rt.SetReg(op.i_type.rt);

    IRValue rs(IRValue::Reg);
    rs.SetReg(op.i_type.rs);

    auto instr = IRInstruction::Build({rs, rt, imm}, IRInstrs::BRANCH);
    instr.b_type = IRInstruction::BranchType::NE;
	instr.is_likely = true;
    curBlock->instructions.push_back(instr);

    printf("bnel %s,%s,pc+%d\n", EmotionEngine::Reg(op.i_type.rs), EmotionEngine::Reg(op.i_type.rt), (int32_t)imm.GetImm());
}

// 0x2B
void EmitSW(Opcode op)
{
    IRValue imm(IRValue::Imm);
    imm.SetImm64(op.i_type.imm);

    IRValue src(IRValue::Reg);
    src.SetReg(op.i_type.rs);

    IRValue dst(IRValue::Reg);
    dst.SetReg(op.i_type.rt);

    auto instr = IRInstruction::Build({dst, imm, src}, IRInstrs::STORE);
    instr.access_size = IRInstruction::AccessSize::U32;
    curBlock->instructions.push_back(instr);

    printf("sw %s, %d(%s)\n", EmotionEngine::Reg(op.i_type.rt), (int16_t)op.i_type.imm, EmotionEngine::Reg(op.i_type.rs));
}

// 0x3F
void EmitSD(Opcode op)
{
    IRValue imm(IRValue::Imm);
    imm.SetImm64(op.i_type.imm);

    IRValue src(IRValue::Reg);
    src.SetReg(op.i_type.rs);

    IRValue dst(IRValue::Reg);
    dst.SetReg(op.i_type.rt);

    auto instr = IRInstruction::Build({dst, imm, src}, IRInstrs::STORE);
    instr.access_size = IRInstruction::AccessSize::U64;
    curBlock->instructions.push_back(instr);

    printf("sd %s, %d(%s)\n", EmotionEngine::Reg(op.i_type.rt), (int16_t)op.i_type.imm, EmotionEngine::Reg(op.i_type.rs));
}

bool IsBranch(Opcode op)
{
    switch (op.opcode)
    {
    case 0x00:
        switch (op.r_type.func)
        {
        case 0x08:
		case 0x09:
			return true;
        default:
            return false;
        }
        break;
	case 0x03:
    case 0x05:
	case 0x14:
	case 0x15:
        return true;
    }

    return false;
}

bool branchDelayed = false;

// Compile and dispatch `cycles` instructions
// If we encounter a branch, return that as the true number of cycles
int EEJit::Clock(int cycles)
{
    curBlock = EEJitX64::GetBlockForAddr(EmotionEngine::GetState()->pc);
    if (curBlock)
    {
    }
    else
    {
        // Create a new block
        curBlock = new Block();
        curBlock->addr = EmotionEngine::GetState()->pc;
        curBlock->instructions.clear();
        curBlock->cycles = 0;

        uint32_t start = curBlock->addr;

        EmitPrologue();

        int cycle = 0;
        int instrs = 0;
        for (; cycle < cycles && instrs < 12; cycle++, instrs++, curBlock->cycles++)
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
                curBlock->instructions.push_back(IRInstruction::Build({}, NOP));
                if (branchDelayed)
                {
                    branchDelayed = false;
                    break;
                }
                continue;
            }

            switch (op.opcode)
            {
            case 0x00:
                EmitSpecial(op);
                break;
			case 0x03:
				EmitJAL(op);
				break;
            case 0x04:
                EmitBEQ(op);
                break;
            case 0x05:
                EmitBNE(op);
                break;
            case 0x09:
                EmitADDIU(op);
                break;
            case 0x0A:
                EmitSLTI(op);
                break;
            case 0x0B:
                EmitSLTIU(op);
                break;
			case 0x0C:
				EmitANDI(op);
				break;
            case 0x0D:
                EmitOri(op);
                break;
            case 0x0F:
                EmitLUI(op);
                break;
            case 0x10:
                EmitCOP0(op);
                break;
			case 0x14:
				EmitBEQL(op);
				break;
			case 0x15:
				EmitBNEL(op);
				break;
            case 0x2B:
                EmitSW(op);
                break;
            case 0x3F:
                EmitSD(op);
                break;
            default:
                printf("[EEJIT]: Cannot emit unknown opcode 0x%02x (0x%08x)\n", op.opcode, op.full);
                exit(1);
            }
            
            if (branchDelayed)
            {
                branchDelayed = false;
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
        EEJitX64::CacheBlock(curBlock);
    }
    // Run it
    curBlock->entryPoint(EmotionEngine::GetState(), curBlock->addr);

	printf("Block returned at pc = 0x%08x\n", EmotionEngine::GetState()->pc);

    return curBlock->cycles;
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
