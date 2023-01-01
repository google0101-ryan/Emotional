#include <emu/cpu/iop/iop.h>
#include <cstdio>
#include <app/Application.h>
#include "iop.h"

void IoProcessor::op_special()
{
	switch (i.r_type.func)
    {
        case 0b000000: op_sll(); break;
        case 0b100101: op_or(); break;
        case 0b101011: op_sltu(); break;
        case 0b100001: op_addu(); break;
        case 0b001000: op_jr(); break;
        case 0b100100: op_and(); break;
        case 0b100000: op_add(); break;
        case 0b001001: op_jalr(); break;
        case 0b100011: op_subu(); break;
        case 0b000011: op_sra(); break;
		case 0b011010: op_div(); break;
        case 0b010010: op_mflo(); break;
        case 0b000010: op_srl(); break;
        case 0b011011: op_divu(); break;
        case 0b010000: op_mfhi(); break;
        case 0b101010: op_slt(); break;
        case 0b001100: op_syscall(); break;
        case 0b010011: op_mtlo(); break;
        case 0b010001: op_mthi(); break;
        case 0b000100: op_sllv(); break;
        case 0b100111: op_nor(); break;
        case 0b000110: op_srlv(); break;
        case 0b011001: op_multu(); break;
        case 0b100110: op_xor(); break;
        case 0b011000: op_mult(); break;
		case 0b000111: op_srav(); break;
		default:
			printf("[emu/IOP]: Unknown special opcode 0x%02x\n", i.r_type.func);
			exit(1);
    }
}

void IoProcessor::op_bcond()
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;

	next_instr.is_delay_slot = true;
	
	bool should_link = (rt & 0x1E) == 0x10;
	bool should_branch = (int)(regs[rs] ^ (rt << 31)) < 0;

	if (should_link) regs[31] = i.pc + 8;
	if (should_branch) branch();

    if (can_disassemble) printf("b%sz %s, 0x%08x\n", rt & 1 ? "lt" : "gt", Reg(rs), pc + ((int16_t)i.i_type.imm << 2));
}

void IoProcessor::op_j()
{
    pc = (pc & 0xF0000000) | (i.j_type.target << 2);

	next_instr.is_delay_slot = true;
	next_instr.branch_taken = true;
    
    if ((pc & 0xFFFF) == 0x1EC8 || (pc & 0xFFFF) == 0x1F64)
    {
        uint32_t struct_ptr = regs[4];
        uint16_t version = bus->read_iop<uint16_t>(struct_ptr + 8);
        char name[9];
        name[8] = 0;
        for (int i = 0; i < 8; i++)
            name[i] = bus->read_iop<uint8_t>(struct_ptr + 12 + i);
        printf("[emu/IOP]: RegisterLibraryEntries: %s version %d.0%d\n", name, version >> 8, version & 0xff);
    }


    if (can_disassemble && i.opcode == 0b000010) printf("j 0x%08x\n", pc);
}

void IoProcessor::op_jal()
{
    set_reg(31, pc);
	op_j();
    if (can_disassemble) printf("jal 0x%08x\n", pc);
}

void IoProcessor::op_beq()
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
	
	next_instr.is_delay_slot = true;
	std::string op = "beq " + std::string(Reg(rs)) + ", " + std::string(Reg(rt));
	if (regs[rs] == regs[rt])
	{
		branch(op);
	}
}

void IoProcessor::op_bne()
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
	
	next_instr.is_delay_slot = true;
	std::string op = "bne " + std::string(Reg(rs)) + ", " + std::string(Reg(rt));
	if (regs[rs] != regs[rt])
	{
		branch(op);
	}
}

void IoProcessor::op_blez()
{
    int rs = i.r_type.rs;

    int32_t reg = (int32_t)regs[rs];

    if (can_disassemble) printf("blez %s, 0x%08x\n", Reg(rs), next_instr.pc + ((int16_t)i.i_type.imm << 2));

    std::string op = "blez " + std::string(Reg(rs));
	if (reg <= 0)
    {
		branch(op);
    }
}

void IoProcessor::op_bgtz()
{
    int rs = i.r_type.rs;

    int32_t reg = (int32_t)regs[rs];
    
	std::string op = "bgez " + std::string(Reg(rs));
	if (reg > 0)
	{
		branch(op);
	}
}

void IoProcessor::op_addi()
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    int16_t imm = (int16_t)i.i_type.imm;

    int32_t reg = (int32_t)regs[rs];
    set_reg(rt, reg + imm);

    if (can_disassemble) printf("addi %s, %s, 0x%04x\n", Reg(rt), Reg(rs), imm);
}

void IoProcessor::op_addiu()
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    int16_t imm = (int16_t)i.i_type.imm;

    set_reg(rt, regs[rs] + imm);

    if (can_disassemble) printf("addiu %s, %s, 0x%04x\n", Reg(rt), Reg(rs), imm);
}

void IoProcessor::op_slti()
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;

    int32_t reg = (int32_t)regs[rs];
    int16_t imm = (int16_t)i.i_type.imm;

	bool condition = reg < imm;

    set_reg(rt, condition);

    if (can_disassemble) printf("slti %s, %s, 0x%04x\n", Reg(rt), Reg(rs), imm);
}

void IoProcessor::op_sltiu()
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;

    uint32_t imm = (int32_t)(int16_t)i.i_type.imm;

	bool condition = regs[rs] < imm;

    set_reg(rt, condition);

    if (can_disassemble) printf("sltiu %s, %s, 0x%04x\n", Reg(rt), Reg(rs), imm);
}

void IoProcessor::op_andi()
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    uint16_t imm = i.i_type.imm;

    if (can_disassemble) printf("andi %s, %s, 0x%04x\n", Reg(rt), Reg(rs), imm);

    set_reg(rt, regs[rs] & imm);
}

void IoProcessor::op_ori()
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    uint16_t imm = i.i_type.imm;

    if (can_disassemble) printf("ori %s, %s, 0x%04x\n", Reg(rt), Reg(rs), imm);

    set_reg(rt, regs[rs] | imm);
}

void IoProcessor::op_lui()
{
    int rt = i.i_type.rt;
    uint32_t imm = i.i_type.imm;

    if (can_disassemble) printf("lui %s, 0x%08x\n", Reg(rt), imm);

    set_reg(rt, imm << 16);
}

void IoProcessor::op_cop0()
{
    switch (i.r_type.rs)
    {
    case 0:
        op_mfc0();
        break;
    case 4:
        op_mtc0();
        break;
	case 0x10:
		op_rfe();
		break;
    default:
        printf("[emu/CPU]: %s: Unknown cop0 opcode 0x%08x (0x%02x)\n", __FUNCTION__, i.full, i.r_type.rs);
        Application::Exit(1);
    }
}

void IoProcessor::op_lb()
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t vaddr = regs[base] + off;

    if (!isCacheIsolated())
    {
        uint32_t value = (int8_t)bus->read_iop<uint8_t>(vaddr);
		load(rt, value);
    }

    if (can_disassemble) printf("lb %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void IoProcessor::op_lh()
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t vaddr = regs[base] + off;

    if (!isCacheIsolated())
    {
		if (vaddr & 1)
		{
			printf("Unaligned LH!\n");
			exit(1);
		}
		uint32_t data = (int16_t)bus->read_iop<uint16_t>(vaddr);
		load(rt, data);
    }

    if (can_disassemble) printf("lh %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void IoProcessor::op_lwl()
{
	static const uint32_t LWL_MASK[4] = { 0xffffff, 0x0000ffff, 0x000000ff, 0x00000000 };
    static const uint8_t LWL_SHIFT[4] = { 24, 16, 8, 0 };

	int16_t offset = (int16_t)(i.i_type.imm);
	uint32_t dest = i.i_type.rt;
	uint32_t base = i.i_type.rs;
	uint32_t addr = regs[base] + offset;
	int shift = addr & 0x3;

	uint32_t mem = bus->read_iop<uint32_t>(addr & ~0x3);
	load(dest, (regs[dest] & LWL_MASK[shift]) | (mem << LWL_SHIFT[shift]));

	if (can_disassemble) printf("lwl %s, %d(%s)\n", Reg(dest), offset, Reg(base));
}

void IoProcessor::op_lw()
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t vaddr = regs[base] + off;

    if (!isCacheIsolated())
    {
		if (vaddr & 3)
		{
			printf("Unaligned LW at address 0x%08x (lw %s, %d(%s))!\n", vaddr, Reg(rt), off, Reg(base));
			exit(1);
		}
		uint32_t data = bus->read_iop<uint32_t>(vaddr);
		load(rt, data);
    }

    if (can_disassemble) printf("lw %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void IoProcessor::op_lbu()
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int32_t off = (int16_t)i.i_type.imm;

    uint32_t vaddr = regs[base] + off;
	
    if (!isCacheIsolated())
    {
        uint32_t value = bus->read_iop<uint8_t>(vaddr);
		load(rt, value);
    }

    if (can_disassemble) printf("lbu %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void IoProcessor::op_lhu()
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t vaddr = regs[base] + off;
	
    if (!isCacheIsolated())
    {
		if (vaddr & 1)
		{
			printf("Unaligned LHU!\n");
			exit(1);
		}
        uint32_t value = bus->read_iop<uint16_t>(vaddr);
		load(rt, value);
    }

    if (can_disassemble) printf("lhu %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void IoProcessor::op_lwr()
{
	static const uint32_t LWR_MASK[4] = { 0x000000, 0xff000000, 0xffff0000, 0xffffff00 };
    static const uint8_t LWR_SHIFT[4] = { 0, 8, 16, 24 };
	int16_t offset = (int16_t)(i.i_type.imm);
	uint32_t dest = i.i_type.rt;
	uint32_t base = i.i_type.rs;
	uint32_t addr = regs[base] + offset;
	int shift = addr & 0x3;

	uint32_t mem = bus->read_iop<uint32_t>(addr & ~0x3);
	mem = (regs[dest] & LWR_MASK[shift]) | (mem >> LWR_SHIFT[shift]);
	load(dest, mem);

	if (can_disassemble) printf("lwr %s, %d(%s)\n", Reg(dest), offset, Reg(base));
}

void IoProcessor::op_sb()
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base] + off;
    
    if (can_disassemble) printf("sb %s, %d(%s)\n", Reg(rt), off, Reg(base));

    if (!isCacheIsolated())
        bus->write_iop<uint8_t>(addr, regs[rt]);
}

void IoProcessor::op_sh()
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base] + off;
    
    if (can_disassemble) printf("sh %s, %d(%s)\n", Reg(rt), off, Reg(base));

	
	if (addr & 1)
	{
		printf("Unaligned SH!\n");
		exit(1);
	}

    if (!isCacheIsolated())
        bus->write_iop<uint16_t>(addr, regs[rt]);
}

void IoProcessor::op_swl()
{
	static const uint32_t SWL_MASK[4] = { 0xffffff00, 0xffff0000, 0xff000000, 0x00000000 };
    static const uint8_t SWL_SHIFT[4] = { 24, 16, 8, 0 };
	int16_t offset = (int16_t)(i.i_type.imm);
	uint32_t source = i.i_type.rt;
	uint32_t base = i.i_type.rs;
	uint32_t addr = regs[base] + offset;
	int shift = addr & 0x3;

	uint32_t mem = bus->read_iop<uint32_t>(addr & ~3);

	bus->write_iop<uint32_t>(addr & ~0x3, (regs[source] >> SWL_SHIFT[shift]) | (mem & SWL_MASK[shift]));
}

void IoProcessor::op_sw()
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base] + off;

	if (addr & 3)
	{
		printf("Unaligned SW!\n");
		exit(1);
	}

    if (can_disassemble) printf("sw %s, %d(%s)\n", Reg(rt), off, Reg(base));
    
    if (!isCacheIsolated())
        bus->write_iop(addr, regs[rt]);
}

void IoProcessor::op_swr()
{
	static const uint32_t SWR_MASK[4] = { 0x00000000, 0x000000ff, 0x0000ffff, 0x00ffffff };
    static const uint8_t SWR_SHIFT[4] = { 0, 8, 16, 24 };
	int16_t offset = (int16_t)(i.i_type.imm);
	uint32_t source = i.i_type.rt;
	uint32_t base = i.i_type.rs;
	uint32_t addr = regs[base] + offset;
	int shift = addr & 0x3;
	uint32_t mem = bus->read_iop<uint32_t>(addr & ~3);

	bus->write_iop<uint32_t>(addr & ~0x3, (regs[source] << SWR_SHIFT[shift]) | (mem & SWR_MASK[shift]));
}

void IoProcessor::op_sll()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int sa = i.r_type.sa;

    set_reg(rd, regs[rt] << sa);

    if (can_disassemble) printf("sll %s, %s, 0x%02x\n", Reg(rd), Reg(rt), sa);
}

void IoProcessor::op_srl()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int sa = i.r_type.sa;

    set_reg(rd, regs[rt] >> sa);

    if (can_disassemble) printf("srl %s, %s, 0x%02x\n", Reg(rd), Reg(rt), sa);
}

void IoProcessor::op_sra()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int sa = i.r_type.sa;

    int32_t reg = (int32_t)regs[rt];
    set_reg(rd, reg >> sa);

    if (can_disassemble) printf("sra %s, %s, 0x%02x\n", Reg(rd), Reg(rt), sa);
}

void IoProcessor::op_sllv()
{
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;
    int rs = i.r_type.rs;

    uint16_t sa = regs[rs] & 0x1F;
    set_reg(rd, regs[rt] << sa);

    if (can_disassemble) printf("sllv %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void IoProcessor::op_srlv()
{
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;
    int rs = i.r_type.rs;

    uint16_t sa = regs[rs] & 0x1F;
    set_reg(rd, regs[rt] >> sa);

    if (can_disassemble) printf("sllv %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void IoProcessor::op_srav()
{
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;
    int rs = i.r_type.rs;

	int32_t source = (int32_t)regs[rs];
	source >>= regs[rs] & 0x1F;
	regs[rd] = (uint32_t)source;

    if (can_disassemble) printf("srav %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void IoProcessor::op_jr()
{
	uint16_t rs = i.i_type.rs;

	pc = regs[rs];
	next_instr.is_delay_slot = true;
	next_instr.branch_taken = true;
    if (can_disassemble) printf("jr %s\n", Reg(i.r_type.rs));
}

void IoProcessor::op_jalr()
{
	uint16_t rd = i.r_type.rd;

	set_reg(rd, i.pc+8);
	op_jr();
    if (can_disassemble) printf("jalr %s, %s\n", Reg(i.r_type.rs), Reg(i.r_type.rd));
}

void IoProcessor::op_syscall()
{
	exception(Exception::Syscall, 0);
}

void IoProcessor::op_addu()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, regs[rs] + regs[rt]);

    if (can_disassemble) printf("addu %s, %s (0x%08x), %s (0x%08x)\n", Reg(rd), Reg(rs), regs[rs], Reg(rt), regs[rt]);
}

void IoProcessor::op_add()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

	uint32_t add = regs[rs] + regs[rt];
    set_reg(rd, add);

    if (can_disassemble) printf("add %s, %s (0x%08x), %s (0x%08x)\n", Reg(rd), Reg(rs), regs[rs], Reg(rt), regs[rt]);
}

void IoProcessor::op_mfhi()
{
    int rd = i.r_type.rd;
	set_reg(rd, hi);
    if (can_disassemble) printf("mfhi %s\n", Reg(rd));
}

void IoProcessor::op_mthi()
{
    int rs = i.r_type.rs;
    hi = regs[rs];
    if (can_disassemble) printf("mthi %s\n", Reg(rs));
}

void IoProcessor::op_mflo()
{
    int rd = i.r_type.rd;
	set_reg(rd, lo);
    if (can_disassemble) printf("mflo %s\n", Reg(rd));
}

void IoProcessor::op_mtlo()
{
    int rs = i.r_type.rs;
    lo = regs[rs];
    if (can_disassemble) printf("mtlo %s\n", Reg(rs));
}

void IoProcessor::op_divu()
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

    if (can_disassemble) printf("divu %s, %s\n", Reg(rs), Reg(rt));
}

void IoProcessor::op_mult()
{
    int rt = i.r_type.rt;
    int rs = i.r_type.rs;

    int64_t result = (int64_t)(int32_t)regs[rs] * (int64_t)(int32_t)regs[rt];

    hi = result >> 32;
    lo = result;

    if (can_disassemble) printf("multu %s, %s\n", Reg(rs), Reg(rt));
}

void IoProcessor::op_multu()
{
    int rt = i.r_type.rt;
    int rs = i.r_type.rs;

    uint64_t result = (uint64_t)regs[rs] * (uint64_t)regs[rt];

    hi = result >> 32;
    lo = result;

    if (can_disassemble) printf("multu %s, %s\n", Reg(rs), Reg(rt));
}

void IoProcessor::op_div()
{
	int rt = i.i_type.rt;
	int rs = i.i_type.rs;

	int32_t dividend = (int32_t)regs[rs];
	int32_t divisor = (int32_t)regs[rt];
	if (divisor == 0) [[unlikely]]
	{
		hi = regs[rs];
		lo = (dividend >= 0 ? 0xFFFFFFFF : 1);
	}
	else if (regs[rs] == 0x80000000 && divisor == -1) [[unlikely]]
	{
		hi = 0;
		lo = 0x80000000;
	}
	else
	{
		hi = (uint32_t)(dividend % divisor);
		lo = (uint32_t)(dividend / divisor);
	}

	if (can_disassemble) printf("div %s, %s\n", Reg(rt), Reg(rs));
}

void IoProcessor::op_subu()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, regs[rs] - regs[rt]);

    if (can_disassemble) printf("subu %s, %s, %s\n", Reg(rd), Reg(rs), Reg(rt));
}

void IoProcessor::op_or()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, regs[rt] | regs[rs]);

    if (can_disassemble) printf("or %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void IoProcessor::op_xor()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, regs[rt] ^ regs[rs]);

    if (can_disassemble) printf("xor %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void IoProcessor::op_nor()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, ~(regs[rt] | regs[rs]));

    if (can_disassemble) printf("nor %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void IoProcessor::op_and()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, regs[rt] & regs[rs]);

    if (can_disassemble) printf("and %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void IoProcessor::op_slt()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    int32_t reg1 = (int32_t)regs[rs];
    int32_t reg2 = (int32_t)regs[rt];
    bool condition = reg1 < reg2;
    set_reg(rd, condition);

    if (can_disassemble) printf("slt %s, %s, %s\n", Reg(rd), Reg(rs), Reg(rt));
}

void IoProcessor::op_sltu()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

	bool condition = regs[rs] < regs[rt];
    set_reg(rd, condition);

    if (can_disassemble) printf("sltu %s, %s, %s\n", Reg(rd), Reg(rs), Reg(rt));
}


void IoProcessor::op_mfc0()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;

	load(rt, Cop0.regs[rd]);

	if (rd == 14)
	{
		//printf("Maybe returning from exception at 0x%08x?\n", Cop0.regs[rd]);
	}
    if (can_disassemble) printf("mfc0 %s, %d\n", Reg(rt), rd);
}

void IoProcessor::op_mtc0()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;

    Cop0.regs[rd] = regs[rt];
 
	if (can_disassemble) printf("mtc0 %d, %s\n", rd, Reg(rt));
}

void IoProcessor::op_rfe()
{
	uint32_t mode = Cop0.status.value & 0x3F;

	Cop0.status.value &= ~(uint32_t)0xF;
	Cop0.status.value |= mode >> 2;

	// printf("rfe\n");
}
