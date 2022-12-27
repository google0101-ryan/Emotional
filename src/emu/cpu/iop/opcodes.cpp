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
		default:
			printf("Unknown special opcode 0x%02x\n", i.r_type.func);
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

    //printf("b%sz %s, 0x%08x\n", rt & 1 ? "lt" : "gt", Reg(rs), pc + (imm << 2));
}

void IoProcessor::op_j()
{
    pc = (pc & 0xF0000000) | (i.j_type.target << 2);

	next_instr.is_delay_slot = true;
	next_instr.branch_taken = true;
    
    if (pc == 0x1EC8 || pc == 0x1F64)
    {
        uint32_t struct_ptr = regs[4];
        uint16_t version = bus->read_iop<uint16_t>(struct_ptr + 8);
        char name[9];
        name[8] = 0;
        for (int i = 0; i < 8; i++)
            name[i] = bus->read_iop<uint8_t>(struct_ptr + 12 + i);
        printf("[emu/IOP]: RegisterLibraryEntries: %s version %d.0%d\n", name, version >> 8, version & 0xff);
    }


    //printf("j 0x%08x\n", next_pc);
}

void IoProcessor::op_jal()
{
    set_reg(31, pc);
	op_j();
    //printf("jal 0x%08x\n", next_pc);
}

void IoProcessor::op_beq()
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
	
	next_instr.is_delay_slot = true;
	if (regs[rs] == regs[rt])
	{
		branch();
	}
}

void IoProcessor::op_bne()
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
	
	next_instr.is_delay_slot = true;
	if (regs[rs] != regs[rt])
	{
		branch();
	}
}

void IoProcessor::op_blez()
{
    int rs = i.r_type.rs;

    int32_t reg = (int32_t)regs[rs];

    //printf("blez %s, 0x%08x\n", Reg(rs), next_pc + (imm << 2));

    if (reg <= 0)
    {
		branch();
    }
}

void IoProcessor::op_bgtz()
{
    int rs = i.r_type.rs;

    int32_t reg = (int32_t)regs[rs];
    
	if (reg > 0)
	{
		branch();
	}
}

void IoProcessor::op_addi()
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    int16_t imm = (int16_t)i.i_type.imm;

    int32_t reg = (int32_t)regs[rs];
    set_reg(rt, reg + imm);

    //printf("addi %s, %s, 0x%04x\n", Reg(rt), Reg(rs), imm);
}

void IoProcessor::op_addiu()
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    int16_t imm = (int16_t)i.i_type.imm;

    set_reg(rt, regs[rs] + imm);

    //printf("addiu %s, %s, 0x%04x\n", Reg(rt), Reg(rs), imm);
}

void IoProcessor::op_slti()
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;

    int32_t reg = (int32_t)regs[rs];
    int16_t imm = (int16_t)i.i_type.imm;

	bool condition = reg < imm;

    set_reg(rt, condition);

    //printf("slti %s, %s, 0x%04x\n", Reg(rt), Reg(rs), imm);
}

void IoProcessor::op_sltiu()
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;

    uint32_t imm = i.i_type.imm;

	bool condition = regs[rs] < imm;

    set_reg(rt, condition);

    //printf("slti %s, %s, 0x%04x\n", Reg(rt), Reg(rs), imm);
}

void IoProcessor::op_andi()
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    uint16_t imm = i.i_type.imm;

    //printf("andi %s, %s, 0x%04x\n", Reg(rt), Reg(rs), imm);

    set_reg(rt, regs[rs] & imm);
}

void IoProcessor::op_ori()
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    uint16_t imm = i.i_type.imm;

    //printf("ori %s, %s, 0x%04x\n", Reg(rt), Reg(rs), imm);

    set_reg(rt, regs[rs] | imm);
}

void IoProcessor::op_lui()
{
    int rt = i.i_type.rt;
    uint32_t imm = i.i_type.imm;

    //printf("lui %s, 0x%08x\n", Reg(rt), imm);

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

    //printf("lb %s, %d(%s)\n", Reg(rt), off, Reg(base));
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

    //printf("lh %s, %d(%s)\n", Reg(rt), off, Reg(base));
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
			printf("Unaligned LW!\n");
			exit(1);
		}
		uint32_t data = bus->read_iop<uint32_t>(vaddr);
		load(rt, data);
    }

    //printf("lw %s, %d(%s)\n", Reg(rt), off, Reg(base));
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

    //printf("lbu %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void IoProcessor::op_lhu()
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t vaddr = regs[base] + off;
	
    if (!isCacheIsolated())
    {
        uint32_t value = bus->read_iop<uint16_t>(vaddr);
		load(rt, value);
    }

    //printf("lhu %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void IoProcessor::op_sb()
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base] + off;
    
    //printf("sb %s, %d(%s)\n", Reg(rt), off, Reg(base));

    if (!isCacheIsolated())
        bus->write_iop<uint8_t>(addr, regs[rt]);
}

void IoProcessor::op_sh()
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base] + off;
    
    //printf("sh %s, %d(%s)\n", Reg(rt), off, Reg(base));

    if (!isCacheIsolated())
        bus->write_iop<uint16_t>(addr, regs[rt]);
}

void IoProcessor::op_sw()
{
    int rt = i.i_type.rt;
    int base = i.i_type.rs;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base] + off;

    //printf("sw %s, %d(%s)\n", Reg(rt), off, Reg(base));
    
    if (!isCacheIsolated())
        bus->write_iop(addr, regs[rt]);
}

void IoProcessor::op_sll()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int sa = i.r_type.sa;

    set_reg(rd, regs[rt] << sa);

    //printf("sll %s, %s, 0x%02x\n", Reg(rd), Reg(rt), sa);
}

void IoProcessor::op_srl()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int sa = i.r_type.sa;

    set_reg(rd, regs[rt] >> sa);

    //printf("srl %s, %s, 0x%02x\n", Reg(rd), Reg(rt), sa);
}

void IoProcessor::op_sra()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int sa = i.r_type.sa;

    int32_t reg = (int32_t)regs[rt];
    set_reg(rd, reg >> sa);

    //printf("sra %s, %s, 0x%02x\n", Reg(rd), Reg(rt), sa);
}

void IoProcessor::op_sllv()
{
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;
    int rs = i.r_type.rs;

    uint16_t sa = regs[rs] & 0x1F;
    set_reg(rd, regs[rt] << sa);

    //printf("sllv %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void IoProcessor::op_srlv()
{
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;
    int rs = i.r_type.rs;

    uint16_t sa = regs[rs] & 0x1F;
    set_reg(rd, regs[rt] >> sa);

    //printf("sllv %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void IoProcessor::op_jr()
{
	uint16_t rs = i.i_type.rs;

	pc = regs[rs];
	next_instr.is_delay_slot = true;
	next_instr.branch_taken = true;
    //printf("jr %s\n", Reg(i.r_type.rs));
}

void IoProcessor::op_jalr()
{
	uint16_t rd = i.r_type.rd;

	set_reg(rd, i.pc+8);
	op_jr();
    //printf("jalr %s, %s\n", Reg(i.r_type.rs), Reg(i.r_type.rd));
}

uint32_t exception_addr[2] = { 0x80000080, 0xBFC00180 };

void IoProcessor::op_syscall()
{
    uint32_t mode = Cop0.status.value & 0x3F;
    Cop0.status.value &= ~(uint32_t)0x3F;
    Cop0.status.value |= (mode << 2) & 0x3F;

	bool is_delay_slot = i.is_delay_slot;
	bool branch_taken = branch_taken;

    Cop0.cause.excode = (uint32_t)8;
    Cop0.cause.CE = 0;

    Cop0.epc = i.pc;

	if (is_delay_slot)
	{
		Cop0.epc -= 4;

		Cop0.cause.BD = true;
		Cop0.TAR = next_instr.pc;

		if (branch_taken)
			Cop0.cause.BT = true;
	}

    pc = exception_addr[Cop0.status.BEV];

    printf("[emu/IOP] syscall\n");
}

void IoProcessor::op_addu()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

	int32_t reg1 = (int32_t)regs[rs];
	int32_t reg2 = (int32_t)regs[rt];

    set_reg(rd, reg1 + reg2);

    //printf("addu %s, %s (0x%08x), %s (0x%08x)\n", Reg(rd), Reg(rs), regs[rs], Reg(rt), regs[rt]);
}

void IoProcessor::op_add()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

	int32_t reg1 = (int32_t)regs[rs];
	int32_t reg2 = (int32_t)regs[rt];

    set_reg(rd, reg1 + reg2);

    //printf("add %s, %s (0x%08x), %s (0x%08x)\n", Reg(rd), Reg(rs), regs[rs], Reg(rt), regs[rt]);
}

void IoProcessor::op_mfhi()
{
    int rd = i.r_type.rd;
    regs[rd] = hi;
    //printf("mfhi %s\n", Reg(rd));
}

void IoProcessor::op_mthi()
{
    int rs = i.r_type.rs;
    hi = regs[rs];
    //printf("mthi %s\n", Reg(rs));
}

void IoProcessor::op_mflo()
{
    int rd = i.r_type.rd;
    regs[rd] = lo;
    //printf("mflo %s\n", Reg(rd));
}

void IoProcessor::op_mtlo()
{
    int rs = i.r_type.rs;
    lo = regs[rs];
    //printf("mtlo %s\n", Reg(rs));
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

    //printf("divu %s, %s\n", Reg(rs), Reg(rt));
}

void IoProcessor::op_mult()
{
    int rt = i.r_type.rt;
    int rs = i.r_type.rs;

    int64_t result = (int64_t)(int32_t)regs[rs] * (int64_t)(int32_t)regs[rt];

    hi = result >> 32;
    lo = result;

    //printf("multu %s, %s\n", Reg(rs), Reg(rt));
}

void IoProcessor::op_multu()
{
    int rt = i.r_type.rt;
    int rs = i.r_type.rs;

    uint64_t result = (uint64_t)regs[rs] * (uint64_t)regs[rt];

    hi = result >> 32;
    lo = result;

    //printf("multu %s, %s\n", Reg(rs), Reg(rt));
}

void IoProcessor::op_subu()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, regs[rs] - regs[rt]);

    //printf("subu %s, %s, %s\n", Reg(rd), Reg(rs), Reg(rt));
}

void IoProcessor::op_or()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, regs[rt] | regs[rs]);

    //printf("or %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void IoProcessor::op_xor()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, regs[rt] ^ regs[rs]);

    //printf("xor %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void IoProcessor::op_nor()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, ~(regs[rt] | regs[rs]));

    //printf("nor %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void IoProcessor::op_and()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, regs[rt] & regs[rs]);

    //printf("and %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
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

    //printf("slt %s, %s, %s\n", Reg(rd), Reg(rs), Reg(rt));
}

void IoProcessor::op_sltu()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    set_reg(rd, regs[rs] < regs[rt]);

    //printf("sltu %s, %s, %s\n", Reg(rd), Reg(rs), Reg(rt));
}


void IoProcessor::op_mfc0()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;

    regs[rt] = Cop0.regs[rd];
    //printf("mfc0 %s, %d\n", Reg(rt), rd);
}

void IoProcessor::op_mtc0()
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;

    Cop0.regs[rd] = regs[rt];
 
	//printf("mtc0 %d, %s\n", rd, Reg(rt));
}