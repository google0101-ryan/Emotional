#include "ee_interpret.h"

#include <util/uint128.h>
#include <emu/memory/Bus.h>
#include <emu/cpu/ee/vu.h>

#define _rs_ ((instr >> 21) & 0x1F)
#define _rt_ ((instr >> 16) & 0x1F)
#define _rd_ ((instr >> 11) & 0x1F)
#define _sa_ ((instr >> 6) & 0x1F)

#define _ft_ ((instr >> 16) & 0x1F)
#define _fs_ ((instr >> 11) & 0x1F)

#define _imm16_ (instr & 0xffff)
#define _simm64_ ((int64_t)(int16_t)(instr & 0xffff))
#define _simm32_ ((int32_t)(int16_t)(instr & 0xffff))
#define _imm26_ (instr & 0x3ffffff)

extern float convert(uint32_t);

extern uint128_t regs[32];
extern uint32_t pc, next_pc;
extern uint32_t hi, lo;
extern uint32_t hi1, lo1;
EEInterpreter::Cop1Reg cop1_regs[32], accumulator;

EETLB *EEInterpreter::tlb;

void EEInterpreter::Mfc0(uint32_t instr)
{
    regs[_rt_].u32[0] = cop0.regs[_rd_];
    if constexpr (CanDisassamble)
        printf("[emu/EE]: mfc0 %s, r%d\n", Reg(_rt_), _rd_);
}

void EEInterpreter::Mtc0(uint32_t instr)
{
    cop0.regs[_rd_] = regs[_rt_].u32[0];

    if constexpr (CanDisassamble)
        printf("[emu/EE]: mtc0 %s, r%d\n", Reg(_rt_), _rd_);
}

void EEInterpreter::Tlbwi(uint32_t)
{
    if constexpr (CanDisassamble)
        printf("[emu/EE]: tlbwi\n");
 
    tlb->DoRemap(cop0.regs[0]);
}

void EEInterpreter::Slti(uint32_t instr)
{
    regs[_rt_].u64[0] = ((int64_t)regs[_rs_].u64[0] < _simm64_);
    if constexpr (CanDisassamble)
        printf("[emu/EE]: slti %s, %s, 0x%04lx\n", Reg(_rt_), Reg(_rs_), _simm64_);
}

void EEInterpreter::Andi(uint32_t instr)
{
    regs[_rt_].u64[0] = regs[_rs_].u64[0] & _imm16_;

    if constexpr (CanDisassamble)
        printf("[emu/EE]: andi %s, %s, 0x%04x\n", Reg(_rt_), Reg(_rs_), _imm16_);
}

void EEInterpreter::Ori(uint32_t instr)
{
    regs[_rt_].u64[0] = regs[_rs_].u64[0] | _imm16_;

    if constexpr (CanDisassamble)
        printf("[emu/EE]: ori %s, %s, 0x%04x\n", Reg(_rt_), Reg(_rs_), _imm16_);
}

void EEInterpreter::Xori(uint32_t instr)
{
    regs[_rt_].u64[0] = regs[_rs_].u64[0] ^ _imm16_;

    if constexpr (CanDisassamble)
        printf("[emu/EE]: xori %s, %s, 0x%04x\n", Reg(_rt_), Reg(_rs_), _imm16_);
}

void EEInterpreter::Lui(uint32_t instr)
{
    regs[_rt_].u64[0] = (_simm64_ << 16);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: lui %s, 0x%08lx\n", Reg(_rt_), (_simm64_ << 16));
}

void EEInterpreter::Beql(uint32_t instr)
{
    if constexpr (CanDisassamble)
        printf("[emu/EE]: beql %s, %s, 0x%08lx\n", Reg(_rs_), Reg(_rt_), next_pc + (_simm64_ << 2));

    if (regs[_rs_].u64[0] == regs[_rt_].u64[0])
        next_pc = pc + (_simm64_ << 2);
    else
    {
        pc = next_pc;
        next_pc += 4;
    }
}

void EEInterpreter::Bnel(uint32_t instr)
{
    if constexpr (CanDisassamble)
        printf("[emu/EE]: bnel %s, %s, 0x%08lx\n", Reg(_rs_), Reg(_rt_), next_pc + (_simm64_ << 2));

    if (regs[_rs_].u64[0] != regs[_rt_].u64[0])
        next_pc = pc + (_simm64_ << 2);
    else
    {
        pc = next_pc;
        next_pc += 4;
    }
}

void EEInterpreter::Daddiu(uint32_t instr)
{
    regs[_rt_].u64[0] = regs[_rs_].u64[0] + _simm64_;

    if constexpr (CanDisassamble)
        printf("[emu/EE]: daddiu %s, %s, 0x%08lx\n", Reg(_rt_), Reg(_rs_), _simm64_);
}

void EEInterpreter::Lq(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;

    regs[_rt_].u128 = Read128(addr);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: lq %s, 0x%08x(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
}

void EEInterpreter::Sq(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;

    Write128(addr, regs[_rt_].u128);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: sq %s, 0x%08x(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
}

void EEInterpreter::Lb(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;

    regs[_rt_].u64[0] = (int8_t)Read8(addr);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: lb %s, 0x%08x(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
}

void EEInterpreter::Lh(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;

    regs[_rt_].u64[0] = (int16_t)Read16(addr);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: lh %s, 0x%08x(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
}

void EEInterpreter::Lw(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;

    regs[_rt_].u64[0] = (int32_t)Read32(addr);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: lw %s, 0x%08x(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
}
    

void EEInterpreter::Lbu(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;
    
    regs[_rt_].u64[0] = Read8(addr);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: lbu %s, 0x%08x(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
}

void EEInterpreter::Lhu(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;
    
    regs[_rt_].u64[0] = Read16(addr);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: lhu %s, 0x%08x(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
}

void EEInterpreter::Lwu(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;
    
    regs[_rt_].u64[0] = Read32(addr);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: lwu %s, 0x%08x(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
}

void EEInterpreter::Sb(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;
    Write8(addr, regs[_rt_].u8[0]);

    if constexpr (CanDisassamble)
       printf("[emu/EE]: sb %s, 0x%08x(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
}

void EEInterpreter::Sh(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;
    Write16(addr, regs[_rt_].u8[0]);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: sh %s, 0x%08x(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
}

void EEInterpreter::Sw(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;
    Write32(addr, regs[_rt_].u32[0]);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: sw %s, 0x%08x(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
}

void EEInterpreter::Swc1(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;

    Write32(addr, cop1_regs[_rt_].u);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: swc1 f%d, 0x%08x(%s)\n", _rt_, _simm32_, Reg(_rs_));
}

void EEInterpreter::Ld(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;

    regs[_rt_].u64[0] = Read64(addr);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: ld %s, 0x%08x(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
}

void EEInterpreter::Sd(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;

    Write64(addr, regs[_rt_].u64[0]);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: sd %s, 0x%08x(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
}

void EEInterpreter::Sll(uint32_t instr)
{
    regs[_rd_].u64[0] = (int64_t)(int32_t)(regs[_rt_].u32[0] << _sa_);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: sll %s, %s, %d\n", Reg(_rd_), Reg(_rt_), _sa_);
}

void EEInterpreter::Srl(uint32_t instr)
{
    regs[_rd_].u64[0] = (int64_t)(regs[_rt_].u32[0] >> _sa_);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: srl %s, %s, %d\n", Reg(_rd_), Reg(_rt_), _sa_);
}

void EEInterpreter::Sra(uint32_t instr)
{
    regs[_rd_].u64[0] = (int64_t)((int32_t)regs[_rt_].u32[0] >> _sa_);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: sra %s, %s, %d\n", Reg(_rd_), Reg(_rt_), _sa_);
}

void EEInterpreter::Sllv(uint32_t instr)
{
    regs[_rd_].u64[0] = (regs[_rt_].u32[0] << (regs[_rs_].u32[0] & 0x3F));

    if constexpr (CanDisassamble)
        printf("[emu/EE]: sllv %s, %s, %s\n", Reg(_rd_), Reg(_rt_), Reg(_rs_));
}

void EEInterpreter::Srav(uint32_t instr)
{
    regs[_rd_].u64[0] = (int64_t)((int32_t)regs[_rt_].u32[0] >> (regs[_rs_].u32[0] & 0x3F));

    if constexpr (CanDisassamble)
        printf("[emu/EE]: srav %s, %s, %s\n", Reg(_rd_), Reg(_rt_), Reg(_rs_));
}

void EEInterpreter::Jr(uint32_t instr)
{
    next_pc = regs[_rs_].u32[0];

    if constexpr (CanDisassamble)
        printf("[emu/EE]: jr %s\n", Reg(_rs_));
}

void EEInterpreter::Jalr(uint32_t instr)
{
    regs[_rd_].u32[0] = next_pc;
    next_pc = regs[_rs_].u32[0];

    if constexpr (CanDisassamble)
        printf("[emu/EE]: jalr %s, %s\n", Reg(_rd_), Reg(_rs_));
}

void EEInterpreter::Movz(uint32_t instr)
{
    if (regs[_rt_].u64[0] == 0) regs[_rd_].u64[0] = regs[_rs_].u64[0];
    
    if constexpr (CanDisassamble)
        printf("[emu/EE]: movz %s, %s, %s\n", Reg(_rd_), Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Movn(uint32_t instr)
{
    if (regs[_rt_].u64[0] != 0) regs[_rd_].u64[0] = regs[_rs_].u64[0];
    
    if constexpr (CanDisassamble)
        printf("[emu/EE]: movn %s, %s, %s\n", Reg(_rd_), Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Sltiu(uint32_t instr)
{
    regs[_rt_].u64[0] = (regs[_rs_].u64[0] < _imm16_);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: sltiu %s, %s, 0x%08lx\n", Reg(_rt_), Reg(_rs_), (uint64_t)_simm64_);
}

void EEInterpreter::Mfhi(uint32_t instr)
{
    regs[_rd_].u64[0] = hi;

    if constexpr (CanDisassamble)
        printf("[emu/EE]: mfhi %s\n", Reg(_rd_));
}

void EEInterpreter::Mflo(uint32_t instr)
{
    regs[_rd_].u64[0] = lo;

    if constexpr (CanDisassamble)
        printf("[emu/EE]: mflo %s\n", Reg(_rd_));
}

void EEInterpreter::Dsllv(uint32_t instr)
{
    regs[_rd_].u64[0] = (regs[_rt_].u64[0] << (regs[_rs_].u32[0] & 0x3F));

    if constexpr (CanDisassamble)
        printf("[emu/EE]: dsllv %s, %s, %s\n", Reg(_rd_), Reg(_rt_), Reg(_rs_));
}

void EEInterpreter::Dsrav(uint32_t instr)
{
    regs[_rd_].u64[0] = ((int64_t)regs[_rt_].u64[0] >> (regs[_rs_].u32[0] & 0x3F));

    if constexpr (CanDisassamble)
        printf("[emu/EE]: dsrav %s, %s, %s\n", Reg(_rd_), Reg(_rt_), Reg(_rs_));
}

void EEInterpreter::Mult(uint32_t instr)
{
    uint64_t result = (int32_t)regs[_rs_].u32[0] * (int32_t)regs[_rt_].u32[0];

    lo = regs[_rd_].u64[0] = (int64_t)(int32_t)result;
    hi = (int32_t)(result >> 32);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: mult %s, %s, %s\n", Reg(_rd_), Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Multu(uint32_t instr)
{
    uint64_t result = regs[_rs_].u32[0] * regs[_rt_].u32[0];

    lo = regs[_rd_].u64[0] = (int64_t)(int32_t)result;
    hi = (int32_t)(result >> 32);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: multu %s, %s, %s\n", Reg(_rd_), Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Div(uint32_t instr)
{
    int64_t numerator = regs[_rs_].u64[0];
    int64_t denominator = regs[_rt_].u64[0];

    if (numerator == (int64_t)0x80000000 && denominator == -1)
    {
        lo = 0x80000000;
        hi = 0xFFFFFFFF;
    }
    else
    {
        lo = numerator / denominator;
        hi = numerator % denominator;
    }

    if constexpr (CanDisassamble)
        printf("[emu/EE]: div %s, %s\n", Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Divu(uint32_t instr)
{
    if (regs[_rt_].u32[0] == 0)
    {
        lo = 0xffffffff;
        hi = regs[_rs_].u32[0];
    }
    else
    {
        lo = regs[_rs_].u32[0] / regs[_rt_].u32[0];
        hi = regs[_rs_].u32[0] % regs[_rt_].u32[0];
    }

    if constexpr (CanDisassamble)
        printf("[emu/EE]: divu %s, %s\n", Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Addu(uint32_t instr)
{
    regs[_rd_].u64[0] = (int32_t)regs[_rs_].u32[0] + (int32_t)regs[_rt_].u32[0];

    if constexpr (CanDisassamble)
        printf("[emu/EE]: addu %s, %s, %s\n", Reg(_rd_), Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Subu(uint32_t instr)
{
    regs[_rd_].u64[0] = (int64_t)regs[_rs_].u64[0] - (int64_t)regs[_rt_].u64[0];

    if constexpr (CanDisassamble)
        printf("[emu/EE]: subu %s, %s, %s\n", Reg(_rd_), Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::And(uint32_t instr)
{
    regs[_rd_].u64[0] = regs[_rs_].u64[0] & regs[_rt_].u64[0];

    if constexpr (CanDisassamble)
        printf("[emu/EE]: and %s, %s, %s\n", Reg(_rd_), Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Or(uint32_t instr)
{
    regs[_rd_].u64[0] = regs[_rs_].u64[0] | regs[_rt_].u64[0];

    if constexpr (CanDisassamble)
        printf("[emu/EE]: or %s, %s, %s\n", Reg(_rd_), Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Nor(uint32_t instr)
{
    regs[_rd_].u64[0] = ~(regs[_rs_].u64[0] | regs[_rt_].u64[0]);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: nor %s, %s, %s\n", Reg(_rd_), Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Daddu(uint32_t instr)
{
    regs[_rd_].u64[0] = regs[_rs_].u64[0] + regs[_rt_].u64[0];

    if constexpr (CanDisassamble)
        printf("[emu/EE]: daddu %s, %s, %s (0x%08lx)\n", Reg(_rd_), Reg(_rs_), Reg(_rt_), regs[_rd_].u64[0]);
}

void EEInterpreter::Dsll(uint32_t instr)
{
    regs[_rd_].u64[0] = (int64_t)(regs[_rt_].u64[0] << _sa_);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: dsll %s, %s, %d\n", Reg(_rd_), Reg(_rt_), _sa_);
}

void EEInterpreter::Dsll32(uint32_t instr)
{
    regs[_rd_].u64[0] = (int64_t)(regs[_rt_].u64[0] << (_sa_ + 32));

    if constexpr (CanDisassamble)
        printf("[emu/EE]: dsll32 %s, %s, %d\n", Reg(_rd_), Reg(_rt_), _sa_);
}

void EEInterpreter::Dsra32(uint32_t instr)
{
    regs[_rd_].u64[0] = ((int64_t)regs[_rt_].u64[0] >> (_sa_ + 32));

    if constexpr (CanDisassamble)
        printf("[emu/EE]: dsra32 %s, %s, %d\n", Reg(_rd_), Reg(_rt_), _sa_);
}

void EEInterpreter::Slt(uint32_t instr)
{
    regs[_rd_].u64[0] = ((int64_t)regs[_rs_].u64[0] < (int64_t)regs[_rt_].u64[0]);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: slt %s, %s, %s\n", Reg(_rd_), Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Sltu(uint32_t instr)
{
    regs[_rd_].u64[0] = (regs[_rs_].u64[0] < regs[_rt_].u64[0]);

    if constexpr (CanDisassamble)
        printf("[emu/EE]: sltu %s, %s, %s\n", Reg(_rd_), Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Bltz(uint32_t instr)
{
    if constexpr (CanDisassamble)
        printf("[emu/EE]: bltz %s, 0x%08lx\n", Reg(_rs_), pc + (_simm64_ << 2));

    if ((int64_t)regs[_rs_].u64[0] < 0)
        next_pc = pc + (_simm64_ << 2);
}

void EEInterpreter::Bgez(uint32_t instr)
{
    if constexpr (CanDisassamble)
        printf("[emu/EE]: bgez %s, 0x%08lx\n", Reg(_rs_), pc + (_simm64_ << 2));

    if ((int64_t)regs[_rs_].u64[0] >= 0)
        next_pc = pc + (_simm64_ << 2);
}


void EEInterpreter::J(uint32_t instr)
{
    next_pc = (next_pc & 0xf0000000) | _imm26_ << 2;

    if constexpr (CanDisassamble)
        printf("[emu/EE]: j 0x%08x\n", next_pc);
}

void EEInterpreter::Jal(uint32_t instr)
{
    regs[31].u32[0] = next_pc;
    next_pc = (next_pc & 0xf0000000) | _imm26_ << 2;

    if constexpr (CanDisassamble)
        printf("[emu/EE]: jal 0x%08x\n", next_pc);
}

void EEInterpreter::Beq(uint32_t instr)
{
    if constexpr (CanDisassamble)
        printf("[emu/EE]: beq %s, %s, 0x%08lx\n", Reg(_rs_), Reg(_rt_), next_pc + (_simm64_ << 2));

    if (regs[_rs_].u64[0] == regs[_rt_].u64[0])
        next_pc = pc + (_simm64_ << 2);
}

void EEInterpreter::Bne(uint32_t instr)
{
    if constexpr (CanDisassamble)
        printf("[emu/EE]: bne %s, %s, 0x%08lx\n", Reg(_rs_), Reg(_rt_), pc + (_simm64_ << 2));

    if (regs[_rs_].u64[0] != regs[_rt_].u64[0])
        next_pc = pc + (_simm64_ << 2);
}

void EEInterpreter::Blez(uint32_t instr)
{
    if constexpr (CanDisassamble)
        printf("[emu/EE]: blez %s, 0x%08lx\n", Reg(_rs_), pc + (_simm64_ << 2));

    if ((int64_t)regs[_rs_].u64[0] <= 0)
        next_pc = pc + (_simm64_ << 2);
}

void EEInterpreter::Bgtz(uint32_t instr)
{
    if constexpr (CanDisassamble)
        printf("[emu/EE]: bgtz %s, 0x%08lx\n", Reg(_rs_), pc + (_simm64_ << 2));

    if ((int64_t)regs[_rs_].u64[0] > 0)
        next_pc = pc + (_simm64_ << 2);
}

void EEInterpreter::Addiu(uint32_t instr)
{
    regs[_rt_].u64[0] = (int32_t)regs[_rs_].u32[0] + _simm32_;

    if constexpr (CanDisassamble)
        printf("[emu/EE]: addiu %s, %s, 0x%08x\n", Reg(_rt_), Reg(_rs_), _simm32_);
}

void EEInterpreter::Mflo1(uint32_t instr)
{
    regs[_rd_].u64[0] = lo1;

    if constexpr (CanDisassamble)
        printf("[emu/EE]: mflo1 %s\n", Reg(_rd_));
}

void EEInterpreter::Divu1(uint32_t instr)
{
    lo1 = regs[_rs_].u32[0] / regs[_rt_].u32[0];
    hi1 = regs[_rs_].u32[0] % regs[_rt_].u32[0];

    if constexpr (CanDisassamble)
        printf("[emu/EE]: divu1 %s, %s\n", Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Mult1(uint32_t instr)
{
    uint64_t result = (int32_t)regs[_rs_].u32[0] * (int32_t)regs[_rt_].u32[0];

    if constexpr (CanDisassamble)
        printf("[emu/EE]: mult1 %s, %s, %s (0x%08x, 0x%08x)\n", Reg(_rd_), Reg(_rs_), Reg(_rt_), (int32_t)regs[_rs_].u32[0], (int32_t)regs[_rt_].u32[0]);

    lo1 = regs[_rd_].u64[0] = (int64_t)(int32_t)result;
    hi1 = (int32_t)(result >> 32);
}

void EEInterpreter::Por(uint32_t instr)
{
    regs[_rd_].u128 = regs[_rs_].u128 | regs[_rt_].u128;

    if constexpr (CanDisassamble)
        printf("[emu/EE]: por %s, %s, %s\n", Reg(_rd_), Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Qmfc2(uint32_t instr)
{
    regs[_rt_].u128 = VectorUnit::VU0::ReadReg(_rd_);
}

void EEInterpreter::Cfc2(uint32_t instr)
{
    regs[_rt_].u64[0] = (int64_t)(int32_t)VectorUnit::VU0::ReadControl(_rd_);
}

void EEInterpreter::Qmtc2(uint32_t instr)
{
   VectorUnit::VU0::WriteReg(_rd_, regs[_rt_].u128);
}

void EEInterpreter::Ctc2(uint32_t instr)
{
    VectorUnit::VU0::WriteControl(_rd_, regs[_rt_].u32[0]);
}

void EEInterpreter::Mtc1(uint32_t instr)
{
    cop1_regs[_rs_].u = regs[_rt_].u32[0];

    if constexpr (CanDisassamble)
        printf("[emu/EE]: mtc1 %s, f%d\n", Reg(_rt_), _rs_);
}

void EEInterpreter::Addas(uint32_t instr)
{
    float op1 = convert(cop1_regs[_fs_].u);
    float op2 = convert(cop1_regs[_ft_].u);
    accumulator.f = op1 + op2;
}