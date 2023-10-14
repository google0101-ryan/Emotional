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

void EEInterpreter::Regimm(uint32_t instr)
{
    switch (_rt_)
    {
    case 0x01:
        Bgez(instr);
        break;
    default:
        printf("[emu/EE]: Unknown REGIMM instruction: 0x%02x\n", _rt_);
        exit(1);
    }
}

void EEInterpreter::J(uint32_t instr)
{
    next_pc = (pc & 0xF0000000) | (_imm26_ << 2);
    if constexpr (CanDisassamble)
        printf("[emu/EE]: j 0x%08x\n", next_pc);
}

void EEInterpreter::Jal(uint32_t instr)
{
    regs[31].u32[0] = pc;
    next_pc = (pc & 0xF0000000) | (_imm26_ << 2);
    if constexpr (CanDisassamble)
        printf("[emu/EE]: jal 0x%08x\n", next_pc);
}

void EEInterpreter::Beq(uint32_t instr)
{
    if (regs[_rs_].u32[0] == regs[_rt_].u32[0])
    {
        next_pc = pc + (_simm32_ << 2);
        if constexpr (CanDisassamble)
            printf("[emu/EE]: beq %s, %s, 0x%x\n", Reg(_rs_), Reg(_rt_), pc+_simm32_);
    }
    else if constexpr (CanDisassamble)
        printf("[emu/EE]: beq %s, %s, 0x%x (not taken)\n", Reg(_rs_), Reg(_rt_), pc+_simm32_);
}

void EEInterpreter::Bne(uint32_t instr)
{
    if (regs[_rs_].u32[0] != regs[_rt_].u32[0])
    {
        next_pc = pc + (_simm32_ << 2);
        if constexpr (CanDisassamble)
            printf("[emu/EE]: bne %s, %s, 0x%08x\n", Reg(_rs_), Reg(_rt_), pc+_simm32_);
    }
    else if constexpr (CanDisassamble)
        printf("[emu/EE]: bne %s, %s, 0x%08x (not taken)\n", Reg(_rs_), Reg(_rt_), pc+_simm32_);
}

void EEInterpreter::Blez(uint32_t instr)
{
    if ((int32_t)regs[_rs_].u32[0] <= 0)
    {
        next_pc = pc + (_simm32_ << 2);
        if constexpr (CanDisassamble)
            printf("[emu/EE]: blez %s, 0x%08x\n", Reg(_rs_), pc+_simm32_);
    }
    else if constexpr (CanDisassamble)
        printf("[emu/EE]: blez %s, 0x%08x (not taken)\n", Reg(_rs_), pc+_simm32_);
}

void EEInterpreter::Bgtz(uint32_t instr)
{
    if ((int32_t)regs[_rs_].u32[0] > 0)
    {
        next_pc = pc + (_simm32_ << 2);
        if constexpr (CanDisassamble)
            printf("[emu/EE]: bgtz %s, 0x%08x\n", Reg(_rs_), pc+_simm32_);
    }
    else if constexpr (CanDisassamble)
        printf("[emu/EE]: bgtz %s, 0x%08x (not taken)\n", Reg(_rs_), pc+_simm32_);
}

void EEInterpreter::Addiu(uint32_t instr)
{
    regs[_rt_].u32[0] = regs[_rs_].u32[0] + _simm32_;
    if constexpr (CanDisassamble)
        printf("[emu/EE]: addiu %s, %s, %d\n", Reg(_rt_), Reg(_rs_), _simm32_);
}

void EEInterpreter::Slti(uint32_t instr)
{
    regs[_rt_].u32[0] = (int32_t)regs[_rs_].u32[0] < (int32_t)_simm32_;
    if constexpr (CanDisassamble)
        printf("[emu/EE]: slti %s, %s, %d\n", Reg(_rt_), Reg(_rs_), _simm32_);
}

void EEInterpreter::Sltiu(uint32_t instr)
{
    regs[_rt_].u32[0] = regs[_rs_].u32[0] < (uint32_t)_simm32_;
    if constexpr (CanDisassamble)
        printf("[emu/EE]: sltiu %s, %s, %d\n", Reg(_rt_), Reg(_rs_), _simm32_);
}

void EEInterpreter::Andi(uint32_t instr)
{
    regs[_rt_].u32[0] = regs[_rs_].u32[0] & _imm16_;
    if constexpr (CanDisassamble)
        printf("[emu/EE]: andi %s, %s, 0x%04x\n", Reg(_rt_), Reg(_rs_), _imm16_);
}

void EEInterpreter::Ori(uint32_t instr)
{
    regs[_rt_].u32[0] = regs[_rs_].u32[0] | _imm16_;
    if constexpr (CanDisassamble)
        printf("[emu/EE]: ori %s, %s, 0x%04x\n", Reg(_rt_), Reg(_rs_), _imm16_);
}

void EEInterpreter::Lui(uint32_t instr)
{
    regs[_rt_].u64[0] = (int64_t)((int32_t)(_imm16_ << 16));
    if constexpr (CanDisassamble)
        printf("[emu/EE]: lui %s, 0x%08x\n", Reg(_rt_), _imm16_ << 16);
}

void EEInterpreter::Cop0(uint32_t instr)
{
    switch ((instr >> 21) & 0x1F)
    {
    case 0x00:
        Mfc0(instr);
        break;
    case 0x04:
        Mtc0(instr);
        break;
    case 0x10:
        TlbOpcodes(instr);
        break;
    default:
        printf("[emu/EE]: Unknown COP0 instruction: 0x%02x\n", (instr >> 21) & 0x1F);
        exit(1);
    }
}

void EEInterpreter::Beql(uint32_t instr)
{
    if (regs[_rs_].u32[0] == regs[_rt_].u32[0])
    {
        next_pc = pc + (_simm32_ << 2);
        if constexpr (CanDisassamble)
            printf("[emu/EE]: beql %s, %s, 0x%08x\n", Reg(_rs_), Reg(_rt_), pc+_simm32_);
    }
    else
    {
        pc = next_pc;
        next_pc = pc + 4;
        if constexpr (CanDisassamble)
            printf("[emu/EE]: beql %s, %s, 0x%08x (not taken)\n", Reg(_rs_), Reg(_rt_), pc+_simm32_);
    }
}

void EEInterpreter::Bnel(uint32_t instr)
{
    if (regs[_rs_].u32[0] != regs[_rt_].u32[0])
    {
        next_pc = pc + (_simm32_ << 2);
        if constexpr (CanDisassamble)
            printf("[emu/EE]: bnel %s, %s, 0x%08x\n", Reg(_rs_), Reg(_rt_), pc+_simm32_);
    }
    else
    {
        pc = next_pc;
        next_pc = pc + 4;
        if constexpr (CanDisassamble)
            printf("[emu/EE]: bnel %s, %s, 0x%08x (not taken)\n", Reg(_rs_), Reg(_rt_), pc+_simm32_);
    }
}

void EEInterpreter::Lb(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;
    uint8_t data = Bus::Read8(addr);
    regs[_rt_].u32[0] = (int32_t)(int8_t)data;
    if constexpr (CanDisassamble)
        printf("[emu/EE]: lb %s, %d(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
}

void EEInterpreter::Lh(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;
    uint16_t data = Bus::Read16(addr);
    regs[_rt_].u32[0] = (int32_t)(int16_t)data;
    if constexpr (CanDisassamble)
        printf("[emu/EE]: lh %s, %d(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
}

void EEInterpreter::Lw(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;
    uint32_t data = Bus::Read32(addr);
    regs[_rt_].u32[0] = data;
    if constexpr (CanDisassamble)
        printf("[emu/EE]: lw %s, %d(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
}

void EEInterpreter::Lbu(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;
    uint8_t data = Bus::Read8(addr);
    regs[_rt_].u32[0] = data;
    if constexpr (CanDisassamble)
        printf("[emu/EE]: lbu %s, %d(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
}

void EEInterpreter::Sb(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;
    uint8_t data = regs[_rt_].u32[0];
    if constexpr (CanDisassamble)
        printf("[emu/EE]: sb %s, %d(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
    Bus::Write8(addr, data);
}

void EEInterpreter::Sw(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;
    uint32_t data = regs[_rt_].u32[0];
    if constexpr (CanDisassamble)
        printf("[emu/EE]: sw %s, %d(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
    Bus::Write32(addr, data);
}

void EEInterpreter::Ld(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;
    uint64_t data = Bus::Read64(addr);
    regs[_rt_].u64[0] = data;
    if constexpr (CanDisassamble)
        printf("[emu/EE]: ld %s, %d(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
}

void EEInterpreter::Swc1(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;
    uint32_t data = cop1_regs[_rt_].u;
    if constexpr (CanDisassamble)
        printf("[emu/EE]: swc1 %s, %d(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
    Bus::Write32(addr, data);
}

void EEInterpreter::Sd(uint32_t instr)
{
    uint32_t addr = regs[_rs_].u32[0] + _simm32_;
    uint64_t data = regs[_rt_].u64[0];
    if constexpr (CanDisassamble)
        printf("[emu/EE]: sd %s, %d(%s)\n", Reg(_rt_), _simm32_, Reg(_rs_));
    Bus::Write64(addr, data);
}

void EEInterpreter::Sll(uint32_t instr)
{
    regs[_rd_].u32[0] = regs[_rt_].u32[0] << _sa_;
    if constexpr (CanDisassamble)
        printf("[emu/EE]: sll %s, %s, %d\n", Reg(_rd_), Reg(_rt_), _sa_);
}

void EEInterpreter::Sra(uint32_t instr)
{
    regs[_rd_].u32[0] = (int32_t)regs[_rt_].u32[0] >> _sa_;
    if constexpr (CanDisassamble)
        printf("[emu/EE]: sra %s, %s, %d\n", Reg(_rd_), Reg(_rt_), _sa_);
}

void EEInterpreter::Jr(uint32_t instr)
{
    next_pc = regs[_rs_].u32[0];
    if constexpr (CanDisassamble)
        printf("[emu/EE]: jr %s\n", Reg(_rs_));
}

void EEInterpreter::Jalr(uint32_t instr)
{
    regs[_rd_].u32[0] = pc;
    next_pc = regs[_rs_].u32[0];
    if constexpr (CanDisassamble)
        printf("[emu/EE]: jalr %s\n", Reg(_rs_));
}

void EEInterpreter::Movn(uint32_t instr)
{
    if (regs[_rt_].u32[0] != 0)
        regs[_rd_].u32[0] = regs[_rs_].u32[0];
    if constexpr (CanDisassamble)
        printf("[emu/EE]: movn %s, %s, %s\n", Reg(_rd_), Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Sync(uint32_t instr)
{
    if constexpr (CanDisassamble)
        printf("[emu/EE]: sync\n");
}

void EEInterpreter::Mfhi(uint32_t instr)
{
    regs[_rd_].u32[0] = hi;
    if constexpr (CanDisassamble)
        printf("[emu/EE]: mfhi %s\n", Reg(_rd_));
}

void EEInterpreter::Mflo(uint32_t instr)
{
    regs[_rd_].u32[0] = lo;
    if constexpr (CanDisassamble)
        printf("[emu/EE]: mflo %s\n", Reg(_rd_));
}

void EEInterpreter::Mult(uint32_t instr)
{
    uint64_t a = regs[_rs_].u64[0];
    uint64_t b = regs[_rt_].u64[0];
    uint64_t res = a * b;
    lo = res & 0xFFFFFFFF;
    hi = res >> 32;
    if constexpr (CanDisassamble)
        printf("[emu/EE]: mult %s, %s\n", Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Div(uint32_t instr)
{
    int32_t a = regs[_rs_].u32[0];
    int32_t b = regs[_rt_].u32[0];

    if (b == 0)
    {
        lo = 0xFFFFFFFF;
        hi = a;
    }
    else if (a == 0x80000000 && b == -1)
    {
        lo = 0x80000000;
        hi = 0;
    }
    else
    {
        lo = a / b;
        hi = a % b;
    }
    if constexpr (CanDisassamble)
        printf("[emu/EE]: div %s, %s\n", Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Divu(uint32_t instr)
{
    uint32_t a = regs[_rs_].u32[0];
    uint32_t b = regs[_rt_].u32[0];

    if (b == 0)
    {
        lo = 0xFFFFFFFF;
        hi = a;
    }
    else
    {
        lo = a / b;
        hi = a % b;
    }
    if constexpr (CanDisassamble)
        printf("[emu/EE]: divu %s, %s\n", Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Addu(uint32_t instr)
{
    regs[_rd_].u32[0] = regs[_rs_].u32[0] + regs[_rt_].u32[0];
    if constexpr (CanDisassamble)
        printf("[emu/EE]: addu %s, %s, %s\n", Reg(_rd_), Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Subu(uint32_t instr)
{
    regs[_rd_].u32[0] = regs[_rs_].u32[0] - regs[_rt_].u32[0];
    if constexpr (CanDisassamble)
        printf("[emu/EE]: subu %s, %s, %s\n", Reg(_rd_), Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Or(uint32_t instr)
{
    regs[_rd_].u32[0] = regs[_rs_].u32[0] | regs[_rt_].u32[0];
    if constexpr (CanDisassamble)
        printf("[emu/EE]: or %s, %s, %s\n", Reg(_rd_), Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Sltu(uint32_t instr)
{
    regs[_rd_].u32[0] = regs[_rs_].u32[0] < regs[_rt_].u32[0];
    if constexpr (CanDisassamble)
        printf("[emu/EE]: sltu %s, %s, %s\n", Reg(_rd_), Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Daddu(uint32_t instr)
{
    regs[_rd_].u64[0] = regs[_rs_].u64[0] + regs[_rt_].u64[0];
    if constexpr (CanDisassamble)
        printf("[emu/EE]: daddu %s, %s, %s\n", Reg(_rd_), Reg(_rs_), Reg(_rt_));
}

void EEInterpreter::Bgez(uint32_t instr)
{
    if ((int32_t)regs[_rs_].u32[0] >= 0)
    {
        next_pc = pc + (_simm32_ << 2);
        if constexpr (CanDisassamble)
            printf("[emu/EE]: bgez %s, 0x%08x\n", Reg(_rs_), pc+_simm32_);
    }
    else if constexpr (CanDisassamble)
        printf("[emu/EE]: bgez %s, 0x%08x (not taken)\n", Reg(_rs_), pc+_simm32_);
}

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

void EEInterpreter::TlbOpcodes(uint32_t instr)
{
    switch (instr & 0x3F)
    {
    case 0x02:
        Tlbwi(instr);
        break;
    default:
        printf("[emu/EE]: Unknown TLB instruction: 0x%02x\n", instr & 0x3F);
        exit(1);
    }
}

void EEInterpreter::Tlbwi(uint32_t instr)
{
    tlb->DoRemap(cop0.index);
    if constexpr (CanDisassamble)
        printf("[emu/EE]: tlbwi\n");
}
