#include <emu/cpu/iop/iop.h>
#include <cstdio>
#include <app/Application.h>

void IoProcessor::bcond(Opcode i)
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    int32_t imm = (int16_t)i.i_type.imm;

    bool should_link = (rt & 0x1E) == 0x10;
    bool should_branch = (int)(regs[rs] ^ (rt << 31)) < 0;

    if (should_link) regs[31] = pc+4;
    if (should_branch) next_pc = pc + (imm << 2);

    //printf("b%sz %s, 0x%08x\n", rt & 1 ? "lt" : "gt", Reg(rs), pc + (imm << 2));
}

void IoProcessor::j(Opcode i)
{
    next_pc = (next_pc & 0xF0000000) | (i.j_type.target << 2);
    //printf("j 0x%08x\n", next_pc);
}

void IoProcessor::jal(Opcode i)
{
    regs[31] = next_pc;
    next_pc = (next_pc & 0xF0000000) | (i.j_type.target << 2);
    //printf("jal 0x%08x\n", next_pc);
}

void IoProcessor::beq(Opcode i)
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    int32_t imm = (int16_t)i.i_type.imm;

    //printf("beq %s, %s, 0x%08x\n", Reg(rs), Reg(rt), pc + (imm << 2));

    if (regs[rs] == regs[rt])
    {
        next_pc = pc + (imm << 2);
    }
}

void IoProcessor::bne(Opcode i)
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    int32_t imm = (int16_t)i.i_type.imm;

    //printf("bne %s, %s, 0x%08x\n", Reg(rs), Reg(rt), pc + (imm << 2));

    if (regs[rs] != regs[rt])
    {
        next_pc = pc + (imm << 2);
    }
}

void IoProcessor::blez(Opcode i)
{
    int rs = i.r_type.rs;

    int32_t reg = (int32_t)regs[rs];
    int32_t imm = (int16_t)i.i_type.imm;

    //printf("blez %s, 0x%08x\n", Reg(rs), next_pc + (imm << 2));

    if (reg <= 0)
    {
        next_pc = pc + (imm << 2);
    }
}

void IoProcessor::bgtz(Opcode i)
{
    int rs = i.r_type.rs;

    int32_t reg = (int32_t)regs[rs];
    int32_t imm = (int16_t)i.i_type.imm;

    //printf("bgtz %s, 0x%08x\n", Reg(rs), next_pc + (imm << 2));

    if (reg > 0)
    {
        next_pc = pc + (imm << 2);
    }
}

void IoProcessor::addi(Opcode i)
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    int16_t imm = (int16_t)i.i_type.imm;

    int32_t reg = (int32_t)regs[rs];
    set_reg(rt, reg + imm);

    //printf("addi %s, %s, 0x%04x\n", Reg(rt), Reg(rs), imm);
}

void IoProcessor::addiu(Opcode i)
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    int16_t imm = (int16_t)i.i_type.imm;

    set_reg(rt, regs[rs] + imm);

    //printf("addiu %s, %s, 0x%04x\n", Reg(rt), Reg(rs), imm);
}

void IoProcessor::slti(Opcode i)
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;

    int32_t reg = (int32_t)regs[rs];
    int32_t imm = (int32_t)i.i_type.imm;

    set_reg(rt, reg < imm);

    //printf("slti %s, %s, 0x%04x\n", Reg(rt), Reg(rs), imm);
}

void IoProcessor::sltiu(Opcode i)
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;

    uint32_t imm = i.i_type.imm;

    set_reg(rt, regs[rs] < imm);

    //printf("slti %s, %s, 0x%04x\n", Reg(rt), Reg(rs), imm);
}

void IoProcessor::andi(Opcode i)
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    uint16_t imm = i.i_type.imm;

    //printf("andi %s, %s, 0x%04x\n", Reg(rt), Reg(rs), imm);

    set_reg(rt, regs[rs] & imm);
}

void IoProcessor::ori(Opcode i)
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    uint16_t imm = i.i_type.imm;

    //printf("ori %s, %s, 0x%04x\n", Reg(rt), Reg(rs), imm);

    set_reg(rt, regs[rs] | imm);
}

void IoProcessor::lui(Opcode i)
{
    int rt = i.i_type.rt;
    uint32_t imm = i.i_type.imm;

    //printf("lui %s, 0x%08x\n", Reg(rt), imm);

    set_reg(rt, imm << 16);
}

void IoProcessor::cop0(Opcode i)
{
    switch (i.r_type.rs)
    {
    case 0:
        mfc0(i);
        break;
    case 4:
        mtc0(i);
        break;
    default:
        //printf("[emu/CPU]: %s: Unknown cop0 opcode 0x%08x (0x%02x)\n", __FUNCTION__, i.full, i.r_type.rs);
        Application::Exit(1);
    }
}

void IoProcessor::lb(Opcode i)
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int32_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base] + off;

    if (!isCacheIsolated())
    {
        next_load_delay.data = (int8_t)bus->read_iop<uint8_t>(addr);
        next_load_delay.reg = rt;
    }

    //printf("lb %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void IoProcessor::lh(Opcode i)
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int32_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base] + off;

    if (!isCacheIsolated())
    {
        next_load_delay.reg = rt;
        next_load_delay.data = (int16_t)bus->read_iop<uint16_t>(addr);
    }

    //printf("lh %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void IoProcessor::lw(Opcode i)
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int32_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base] + off;


    if (!isCacheIsolated())
    {
        next_load_delay.data = bus->read_iop<uint32_t>(addr);
        next_load_delay.reg = rt;
    }

    //printf("lw %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void IoProcessor::lbu(Opcode i)
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int32_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base] + off;

    if (!isCacheIsolated())
    {
        next_load_delay.data = bus->read_iop<uint8_t>(addr);
        next_load_delay.reg = rt;
    }

    //printf("lbu %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void IoProcessor::lhu(Opcode i)
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int32_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base] + off;

    if (!isCacheIsolated())
    {
        next_load_delay.data = bus->read_iop<uint16_t>(addr);
        next_load_delay.reg = rt;
    }

    //printf("lhu %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void IoProcessor::sb(Opcode i)
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int32_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base] + off;
    
    //printf("sb %s, %d(%s)\n", Reg(rt), off, Reg(base));

    if (!isCacheIsolated())
        bus->write_iop<uint8_t>(addr, regs[rt]);
}

void IoProcessor::sh(Opcode i)
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int32_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base] + off;
    
    //printf("sh %s, %d(%s)\n", Reg(rt), off, Reg(base));

    if (!isCacheIsolated())
        bus->write_iop<uint16_t>(addr, regs[rt]);
}

void IoProcessor::sw(Opcode i)
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int32_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base] + off;

    //printf("sw %s, %d(%s)\n", Reg(rt), off, Reg(base));
    
    if (!isCacheIsolated())
        bus->write_iop(addr, regs[rt]);
}

void IoProcessor::sll(Opcode i)
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int sa = i.r_type.sa;

    set_reg(rd, regs[rt] << sa);

    //printf("sll %s, %s, 0x%02x\n", Reg(rd), Reg(rt), sa);
}

void IoProcessor::srl(Opcode i)
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int sa = i.r_type.sa;

    set_reg(rd, regs[rt] >> sa);

    //printf("srl %s, %s, 0x%02x\n", Reg(rd), Reg(rt), sa);
}

void IoProcessor::sra(Opcode i)
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int sa = i.r_type.sa;

    int32_t reg = (int32_t)regs[rt];
    set_reg(rd, reg >> sa);

    //printf("sra %s, %s, 0x%02x\n", Reg(rd), Reg(rt), sa);
}

void IoProcessor::sllv(Opcode i)
{
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;
    int rs = i.r_type.rs;

    uint16_t sa = regs[rs] & 0x1F;
    set_reg(rd, regs[rt] << sa);

    //printf("sllv %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void IoProcessor::srlv(Opcode i)
{
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;
    int rs = i.r_type.rs;

    uint16_t sa = regs[rs] & 0x1F;
    set_reg(rd, regs[rt] >> sa);

    //printf("sllv %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void IoProcessor::jr(Opcode i)
{
    next_pc = regs[i.r_type.rs];
    //printf("jr %s\n", Reg(i.r_type.rs));
}

void IoProcessor::jalr(Opcode i)
{
    regs[i.r_type.rd] = next_pc;
    next_pc = regs[i.r_type.rs];
    //printf("jalr %s, %s\n", Reg(i.r_type.rs), Reg(i.r_type.rd));
}

void IoProcessor::syscall(Opcode)
{
    uint32_t mode = Cop0.regs[12] & 0x3F;
    Cop0.regs[12] &= ~(uint32_t)0x3F;
    Cop0.regs[12] |= (mode << 2) & 0x3F;

    Cop0.regs[13] |= (uint32_t)(0x8 << 2);
    Cop0.regs[13] &= (0 << 28);

    Cop0.regs[14] = next_pc;

    bool bev = Cop0.regs[12] & (1 << 22);
    pc = bev ? 0x80000080 : 0xBFC00180;

    //printf("syscall\n");
}

void IoProcessor::addu(Opcode i)
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, regs[rs] + regs[rt]);

    //printf("addu %s, %s (0x%08x), %s (0x%08x)\n", Reg(rd), Reg(rs), regs[rs], Reg(rt), regs[rt]);
}

void IoProcessor::add(Opcode i)
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, regs[rs] + regs[rt]);

    //printf("add %s, %s (0x%08x), %s (0x%08x)\n", Reg(rd), Reg(rs), regs[rs], Reg(rt), regs[rt]);
}

void IoProcessor::mfhi(Opcode i)
{
    int rd = i.r_type.rd;
    regs[rd] = hi;
    //printf("mfhi %s\n", Reg(rd));
}

void IoProcessor::mthi(Opcode i)
{
    int rs = i.r_type.rs;
    hi = regs[rs];
    //printf("mthi %s\n", Reg(rs));
}

void IoProcessor::mflo(Opcode i)
{
    int rd = i.r_type.rd;
    regs[rd] = lo;
    //printf("mflo %s\n", Reg(rd));
}

void IoProcessor::mtlo(Opcode i)
{
    int rs = i.r_type.rs;
    lo = regs[rs];
    //printf("mtlo %s\n", Reg(rs));
}

void IoProcessor::divu(Opcode i)
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;

    uint32_t dividend = regs[rs];
    uint32_t divisor = regs[rt];
    if (divisor == 0) [[unlikely]]
    {
        hi = dividend;
        lo = 0xFFFFFFFF;
    }
    else
    {
        hi = dividend % divisor;
        lo = dividend / divisor;
    }

    //printf("divu %s, %s\n", Reg(rs), Reg(rt));
}

void IoProcessor::mult(Opcode i)
{
    int rt = i.r_type.rt;
    int rs = i.r_type.rs;

    int64_t result = (int64_t)(int32_t)regs[rs] * (int64_t)(int32_t)regs[rt];

    hi = result >> 32;
    lo = result;

    //printf("multu %s, %s\n", Reg(rs), Reg(rt));
}

void IoProcessor::multu(Opcode i)
{
    int rt = i.r_type.rt;
    int rs = i.r_type.rs;

    uint64_t result = (uint64_t)regs[rs] * (uint64_t)regs[rt];

    hi = result >> 32;
    lo = result;

    //printf("multu %s, %s\n", Reg(rs), Reg(rt));
}

void IoProcessor::subu(Opcode i)
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, regs[rs] - regs[rt]);

    //printf("subu %s, %s, %s\n", Reg(rd), Reg(rs), Reg(rt));
}

void IoProcessor::op_or(Opcode i)
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, regs[rt] | regs[rs]);

    //printf("or %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void IoProcessor::op_xor(Opcode i)
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, regs[rt] ^ regs[rs]);

    //printf("xor %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void IoProcessor::op_nor(Opcode i)
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, ~(regs[rt] | regs[rs]));

    //printf("nor %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void IoProcessor::op_and(Opcode i)
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, regs[rt] & regs[rs]);

    //printf("and %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void IoProcessor::slt(Opcode i)
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    int32_t reg1 = (int32_t)regs[rs];
    int32_t reg2 = (int32_t)regs[rt];
    bool condition = reg1 < reg2;
    set_reg(rd, condition);

    //printf("slt %s, %s, %s\n", Reg(rd), Reg(rs), Reg(rt));
}

void IoProcessor::sltu(Opcode i)
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, regs[rs] < regs[rt]);

    //printf("sltu %s, %s, %s\n", Reg(rd), Reg(rs), Reg(rt));
}


void IoProcessor::mfc0(Opcode i)
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;

    regs[rt] = Cop0.regs[rd];
    //printf("mfc0 %s, %d\n", Reg(rt), rd);
}

void IoProcessor::mtc0(Opcode i)
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;

    Cop0.regs[rd] = regs[rt];
    //printf("mtc0 %d, %s\n", rd, Reg(rt));
}