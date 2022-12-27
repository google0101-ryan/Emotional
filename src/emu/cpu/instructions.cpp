#include "emu/cpu/opcode.h"
#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <cstdio>
#include <emu/cpu/EmotionEngine.h>
#include <app/Application.h>
#include "EmotionEngine.h"


void EmotionEngine::j(Opcode i)
{
    uint32_t target = i.j_type.target;
    pc = ((instr.pc + 4) & 0xF0000000) | (target << 2);

	next_instr.is_delay_slot = true;
	branch_taken = true;
    if (can_disassemble) printf("j 0x%08x\n", pc);
}

void EmotionEngine::jal(Opcode i)
{
    uint32_t target = i.j_type.target;
    regs[31].u64[0] = instr.pc + 8;
    pc = ((instr.pc + 4) & 0xF0000000) | (target << 2);

	next_instr.is_delay_slot = true;
	branch_taken = true;
    if (can_disassemble) printf("jal 0x%08x (0x%08x)\n", pc, regs[31].u64[0]);
}

void EmotionEngine::beq(Opcode i)
{
    uint16_t rt = i.i_type.rt;
    uint16_t rs = i.i_type.rs;
    int32_t imm = (int16_t)i.i_type.imm;

    int32_t offset = imm << 2;
    if (regs[rs].u64[0] == regs[rt].u64[0])
    {
        pc = instr.pc + offset + 4;
		branch_taken = true;
    }

	next_instr.is_delay_slot = true;

    if (can_disassemble) printf("beq %s, %s, 0x%08x%s\n", Reg(rs), Reg(rt), instr.pc + offset, branch_taken ? " [taken]" : "");
}

void EmotionEngine::bne(Opcode i)
{
    uint16_t rt = i.i_type.rt;
    uint16_t rs = i.i_type.rs;
    int32_t imm = (int16_t)i.i_type.imm;

    int32_t offset = imm << 2;
    if (regs[rs].u64[0] != regs[rt].u64[0])
    {
		branch_taken = true;
        pc = instr.pc + offset + 4;
    }

	instr.is_delay_slot = true;

    if (can_disassemble) printf("beq %s, %s, 0x%08x\n", Reg(rs), Reg(rt), instr.pc + offset);
}

void EmotionEngine::blez(Opcode i)
{
    int32_t imm = (int16_t)i.i_type.imm;
    uint16_t rs = i.i_type.rs;

    int32_t offset = imm << 2;
    int64_t reg = regs[rs].u64[0];
    if (reg <= 0)
    {
        pc = instr.pc + offset + 4;
		branch_taken = true;
    }

	next_instr.is_delay_slot = true;

    if (can_disassemble) printf("blez %s, 0x%08x\n", Reg(rs), instr.pc + offset);
}

void EmotionEngine::bgtz(Opcode i)
{
    int32_t off = (int32_t)((int16_t)(i.i_type.imm));
    int32_t imm = off << 2;

    int rs = i.r_type.rs;

    if (can_disassemble) printf("bgtz %s, 0x%08x\n", Reg(rs), instr.pc + 4 + imm);

    int64_t reg = regs[rs].u64[0];

    if (reg > 0)
    {
		branch_taken = true;
        pc = instr.pc + 4 + imm;
    }

	next_instr.is_delay_slot = true;
}

void EmotionEngine::addiu(Opcode i)
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    int16_t imm = (int16_t)i.i_type.imm;
	int32_t reg = (int32_t)regs[rs].u32[0];

    int32_t result = reg + imm;
    regs[rt].u64[0] = result;

    if (can_disassemble) printf("addiu %s, %s, %d\n", Reg(rt), Reg(rs), imm);
}

void EmotionEngine::slti(Opcode i)
{
    int rs = i.i_type.rs;
    int rt = i.i_type.rt;
    int64_t imm = (int16_t)i.i_type.imm;

    int64_t reg = regs[rs].u64[0];
    regs[rt].u64[0] = reg < imm;

    if (can_disassemble) printf("slti %s, %s, 0x%04x\n", Reg(i.i_type.rt), Reg(i.i_type.rs), i.i_type.imm);
}

void EmotionEngine::sltiu(Opcode i)
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;
    uint64_t imm = i.i_type.imm;

    regs[rt].u64[0] = regs[rs].u64[0] < imm;

    if (can_disassemble) printf("sltiu %s, %s, 0x%04x\n", Reg(i.i_type.rt), Reg(i.i_type.rs), i.i_type.imm);
}

void EmotionEngine::andi(Opcode i)
{
    int rt = i.i_type.rt;
	int rs = i.i_type.rs;
	uint64_t imm = i.i_type.imm;

	regs[rt].u64[0] = regs[rs].u64[0] & imm;
	
	if (can_disassemble) printf("andi %s, %s, 0x%08x\n", Reg(i.i_type.rt), Reg(i.i_type.rs), i.i_type.imm);
}

void EmotionEngine::ori(Opcode i)
{
    int rt = i.i_type.rt;
	int rs = i.i_type.rs;
	uint64_t imm = i.i_type.imm;

	regs[rt].u64[0] = regs[rs].u64[0] | imm;
	
	if (can_disassemble) printf("ori %s, %s, 0x%08x\n", Reg(i.i_type.rt), Reg(i.i_type.rs), i.i_type.imm);
}

void EmotionEngine::xori(Opcode i)
{
    int rt = i.i_type.rt;
	int rs = i.i_type.rs;
	uint64_t imm = i.i_type.imm;

	regs[rt].u64[0] = regs[rs].u64[0] ^ imm;
    
	if (can_disassemble) printf("xori %s, %s, 0x%08x\n", Reg(i.i_type.rt), Reg(i.i_type.rs), i.i_type.imm);
}

void EmotionEngine::lui(Opcode i)
{
	int rt = i.i_type.rt;
    uint32_t imm = (i.i_type.imm << 16);
	int64_t value = (int32_t)imm;
	
	regs[rt].u64[0] = value;

    if (can_disassemble) printf("lui %s, 0x%08lx\n", Reg(i.i_type.rt), regs[i.i_type.rt].u64[0]);
}

void EmotionEngine::mfc0(Opcode i)
{
    regs[i.r_type.rt].u64[0] = cop0_regs[i.r_type.rd];
    if (can_disassemble) printf("mfc0 %s, r%d\n", Reg(i.r_type.rt), i.r_type.rd);
}

void EmotionEngine::mtc0(Opcode i)
{
    cop0_regs[i.r_type.rd] = regs[i.r_type.rt].u32[0];
    if (can_disassemble) printf("mtc0 %s, r%d (0x%08x)\n", Reg(i.r_type.rt), i.r_type.rd, regs[i.r_type.rt].u32[0]);
}

void EmotionEngine::eret(Opcode i)
{
	uint32_t status = cop0_regs[12];
	bool erl = (status >> 2) & 1;

	if (erl)
	{
		pc = cop0_regs[30];
		cop0_regs[12] &= ~(1 << 2);
	}
	else
	{
		pc = cop0_regs[14];
		cop0_regs[12] &= ~(1 << 1);
	}

	branch_taken = true;

	if (can_disassemble)
		printf("eret (0x%08x)\n", i.full, pc);

	//can_disassemble = true;

	fetch_next();
}

void EmotionEngine::ei(Opcode i)
{
	uint32_t status = cop0_regs[12];
	bool edi = (status >> 17) & 1;
	bool exl = (status >> 1) & 1;
	bool erl = (status >> 2) & 1;
	int ksu = (status >> 3) & 2;

	if (edi || exl || erl || !ksu)
	{
		cop0_regs[12] |= (1 << 16);
	}

	if (can_disassemble) printf("ei\n");
}

void EmotionEngine::di(Opcode i)
{
	uint32_t status = cop0_regs[12];
	bool edi = (status >> 17) & 1;
	bool exl = (status >> 1) & 1;
	bool erl = (status >> 2) & 1;
	int ksu = (status >> 3) & 2;

	if (edi || exl || erl || !ksu)
	{
		cop0_regs[12] &= ~(1 << 16);
	}

	if (can_disassemble) printf("di\n");
}
void EmotionEngine::beql(Opcode i)
{
    int32_t off = (int16_t)i.i_type.imm;
    int32_t imm = off << 2;

    if (can_disassemble) printf("beql %s, %s, 0x%08x\n", Reg(i.i_type.rs), Reg(i.i_type.rt), pc + imm);

    if (regs[i.i_type.rs].u64[0] == regs[i.i_type.rt].u64[0])
    {
        pc = instr.pc + 4 + imm;
		branch_taken = true;
    }
    else
        fetch_next(); // Skip delay slot
}

void EmotionEngine::bnel(Opcode i)
{
    int32_t off = (int16_t)i.i_type.imm;
    int32_t imm = off << 2;

    bool taken = false;

    if (regs[i.i_type.rs].u64[0] != regs[i.i_type.rt].u64[0])
    {
        pc = instr.pc + 4 + imm;
        taken = true;
    }
    else
        fetch_next(); // Skip delay slot
    
    if (can_disassemble) printf("bnel %s, %s, 0x%08x (%s)\n", Reg(i.i_type.rs), Reg(i.i_type.rt), instr.pc + 4 + imm, taken ? "taken" : "");
}

void EmotionEngine::ldl(Opcode i)
{
    static const uint64_t LDL_MASK[8] =
    {
        0x00ffffffffffffffULL, 0x0000ffffffffffffULL, 0x000000ffffffffffULL, 0x00000000ffffffffULL,
        0x0000000000ffffffULL, 0x000000000000ffffULL, 0x00000000000000ffULL, 0x0000000000000000ULL
    };
    static const uint8_t LDL_SHIFT[8] = { 56, 48, 40, 32, 24, 16, 8, 0 };

    uint16_t rt = i.i_type.rt;
    uint16_t base = i.i_type.rs;
    int16_t offset = (int16_t)i.i_type.imm;

    /* The address given is unaligned, so let's align it first */
    uint32_t addr = offset + regs[base].u32[0];
    uint32_t aligned_addr = addr & ~0x7;
    uint32_t shift = addr & 0x7;

	auto dword = bus->read<uint64_t>(aligned_addr);
	uint64_t result = (regs[rt].u64[0] & LDL_MASK[shift]) | (dword << LDL_SHIFT[shift]);
	regs[rt].u64[0] = result;

    if (can_disassemble) printf("ldl %s, %s(0x%08x)\n", Reg(rt), Reg(base), offset);
}

void EmotionEngine::ldr(Opcode i)
{
    static const uint64_t LDR_MASK[8] =
    {
        0x0000000000000000ULL, 0xff00000000000000ULL, 0xffff000000000000ULL, 0xffffff0000000000ULL,
        0xffffffff00000000ULL, 0xffffffffff000000ULL, 0xffffffffffff0000ULL, 0xffffffffffffff00ULL
    };
    static const uint8_t LDR_SHIFT[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };

    uint16_t rt = i.i_type.rt;
    uint16_t base = i.i_type.rs;
    int16_t offset = (int16_t)i.i_type.imm;

    /* The address given is unaligned, so let's align it first */
    uint32_t addr = offset + regs[base].u32[0];
    uint32_t aligned_addr = addr & ~0x7;
    uint16_t shift = addr & 0x7;

    auto dword = bus->read<uint64_t>(aligned_addr);
    uint64_t result = (regs[rt].u64[0] & LDR_MASK[shift]) | (dword >> LDR_SHIFT[shift]);
    regs[rt].u64[0] = result;

    if (can_disassemble) printf("ldr %s, %s(0x%08x)\n", Reg(rt), Reg(base), offset);
}

void EmotionEngine::daddiu(Opcode i)
{
    int rs = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t offset = (int16_t)i.i_type.imm;

    int64_t reg = regs[rs].u64[0];
    regs[rt].u64[0] = reg + offset;

    if (can_disassemble) printf("daddiu %s, %s, 0x%04x\n", Reg(rt), Reg(rs), offset);
}

void EmotionEngine::lq(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    regs[rt].u128 = bus->read<__uint128_t>(addr);

    if (can_disassemble) printf("lq %s, %d(%s(0x%x))\n", Reg(rt), off, Reg(base), regs[base].u32[0]);
}

void EmotionEngine::sq(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    bus->write(addr, regs[rt].u128);

    if (can_disassemble) printf("sq %s, %d(%s(0x%x))\n", Reg(rt), off, Reg(base), regs[base].u32[0]);
}

void EmotionEngine::lb(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int32_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    regs[rt].u64[0] = (int8_t)bus->read<uint8_t>(addr);

    if (can_disassemble) printf("lb %s, %d(%s(%s))\n", Reg(rt), off, Reg(base), print_128(regs[rt]));
}

void EmotionEngine::lh(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    regs[rt].u64[0] = (int16_t)bus->read<uint16_t>(addr);

    if (can_disassemble) printf("lh %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void EmotionEngine::lw(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = off + regs[base].u32[0];

    if (addr & 0x3)
    {
        printf("lw: Unaligned address\n");
        exit(1);
    }

    regs[rt].u64[0] = (int64_t)bus->read<int32_t>(addr);

    if (can_disassemble) printf("lw %s, %d(%s(0x%x))\n", Reg(rt), off, Reg(base), addr);
}

void EmotionEngine::lbu(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    regs[rt].u64[0] = bus->read<uint8_t>(addr);

    if (can_disassemble) printf("lbu %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void EmotionEngine::lhu(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    regs[rt].u64[0] = bus->read<uint16_t>(addr);

    if (can_disassemble) printf("lhu %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void EmotionEngine::lwu(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    regs[rt].u64[0] = bus->read<uint32_t>(addr);

    if (can_disassemble) printf("lwu %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void EmotionEngine::sb(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    bus->write<uint8_t>(addr, regs[rt].u32[0] & 0xFF);

    if (can_disassemble) printf("sb %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void EmotionEngine::sh(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    bus->write<uint16_t>(addr, regs[rt].u16[0]);

    if (can_disassemble) printf("sh %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void EmotionEngine::sw(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int32_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    if (can_disassemble) printf("sw %s, %d(%s) -> (0x%08x, 0x%08x)\n", Reg(rt), off, Reg(base), addr, instr.pc);

    bus->write<uint32_t>(addr, regs[rt].u32[0]);
}

void EmotionEngine::sdl(Opcode i)
{
    static const uint64_t SDL_MASK[8] =
    {
        0xffffffffffffff00ULL, 0xffffffffffff0000ULL, 0xffffffffff000000ULL, 0xffffffff00000000ULL,
        0xffffff0000000000ULL, 0xffff000000000000ULL, 0xff00000000000000ULL, 0x0000000000000000ULL
    };
    static const uint8_t SDL_SHIFT[8] = { 56, 48, 40, 32, 24, 16, 8, 0 };

    uint16_t rt = i.i_type.rt;
    uint16_t base = i.i_type.rs;
    int16_t offset = (int16_t)i.i_type.imm;

    /* The address given is unaligned, so let's align it first */
    uint32_t addr = offset + regs[base].u32[0];
    uint32_t aligned_addr = addr & ~0x7;
    uint32_t shift = addr & 0x7;

    auto dword = bus->read<uint64_t>(aligned_addr);
    dword = (regs[rt].u64[0] >> SDL_SHIFT[shift]) | (dword & SDL_MASK[shift]);
    bus->write<uint64_t>(aligned_addr, dword);

    if (can_disassemble) printf("sdl %s, %s(0x%08x)\n", Reg(rt), Reg(base), offset);
}

void EmotionEngine::sdr(Opcode i)
{
    static const uint64_t SDR_MASK[8] =
    {
        0x0000000000000000ULL, 0x00000000000000ffULL, 0x000000000000ffffULL, 0x0000000000ffffffULL,
        0x00000000ffffffffULL, 0x000000ffffffffffULL, 0x0000ffffffffffffULL, 0x00ffffffffffffffULL
    };
    static const uint8_t SDR_SHIFT[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };

    uint16_t rt = i.i_type.rt;
    uint16_t base = i.i_type.rs;
    int16_t offset = (int16_t)i.i_type.imm;

    /* The address given is unaligned, so let's align it first */
    uint32_t addr = offset + regs[base].u32[0];
    uint32_t aligned_addr = addr & ~0x7;
    uint32_t shift = addr & 0x7;

    auto dword = bus->read<uint64_t>(aligned_addr);
    dword = (regs[rt].u64[0] << SDR_SHIFT[shift]) | (dword & SDR_MASK[shift]);
    bus->write<uint64_t>(aligned_addr, dword);

    if (can_disassemble) printf("sdr %s, %s(0x%08x)\n", Reg(rt), Reg(base), offset);
}

void EmotionEngine::ld(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    regs[rt].u64[0] = bus->read<uint64_t>(addr);

    if (can_disassemble) printf("ld %s, %d(%s)\n", Reg(rt), off, Reg(base));
}

void EmotionEngine::swc1(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    bus->write<uint32_t>(addr, cop1.regs.u[rt]);

    if (can_disassemble) printf("swc1 f%d, %d(%s)\n", rt, off, Reg(base));
}

void EmotionEngine::sd(Opcode i)
{
    int base = i.i_type.rs;
    int rt = i.i_type.rt;
    int16_t off = (int16_t)i.i_type.imm;

    uint32_t addr = regs[base].u32[0] + off;

    bus->write<uint64_t>(addr, regs[rt].u64[0]);

    if (can_disassemble) printf("sd %s, %d(%s)\n", Reg(rt), off, Reg(base));
}


void EmotionEngine::jr(Opcode i)
{
    pc = regs[i.r_type.rs].u32[0];

	next_instr.is_delay_slot = true;
	branch_taken = true;
    if (can_disassemble) printf("jr %s (0x%08x)\n", Reg(i.r_type.rs), pc);
}


void EmotionEngine::sll(Opcode i)
{
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;
    int sa = i.r_type.sa;

    regs[rd].u64[0] = (uint64_t)(int32_t)(regs[rt].u32[0] << sa);

    if (i.full != 0)
	{
        if (can_disassemble) printf("sll %s, %s, %d\n", Reg(rd), Reg(rt), sa);
	}
    else
	{
        if (can_disassemble) printf("nop\n");
	}
}

void EmotionEngine::srl(Opcode i)
{
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;
    int sa = i.r_type.sa;

    regs[rd].u64[0] = (int32_t)(regs[rt].u32[0] >> sa);

    if (can_disassemble) printf("srl %s, %s, %d\n", Reg(rd), Reg(rt), sa);
}

void EmotionEngine::sra(Opcode i)
{
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;
    int sa = i.r_type.sa;

    int32_t reg = (int32_t)regs[rt].u32[0];
    regs[rd].u64[0] = reg >> sa;

    if (can_disassemble) printf("sra %s, %s, %d\n", Reg(rd), Reg(rt), sa);
}

void EmotionEngine::sllv(Opcode i)
{
    int rs = i.r_type.rs;
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;

    uint32_t reg = regs[rt].u32[0];
    uint16_t sa = regs[rs].u32[0] & 0x1f;
    regs[rd].u64[0] = (int32_t)(reg << sa);

    if (can_disassemble) printf("sllv %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void EmotionEngine::srlv(Opcode i)
{
    int rs = i.r_type.rs;
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;

    uint16_t sa = regs[rs].u32[0] & 0x1f;
    regs[rd].u64[0] = (int32_t)(regs[rt].u32[0] >> sa);

    if (can_disassemble) printf("srlv %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void EmotionEngine::srav(Opcode i)
{
    int rs = i.r_type.rs;
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;

    int32_t reg = (int32_t)regs[rt].u32[0];
    uint16_t sa = regs[rs].u32[0] & 0x1f;
    regs[rd].u64[0] = reg >> sa;

    if (can_disassemble) printf("srav %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void EmotionEngine::jalr(Opcode i)
{
    int rs = i.r_type.rs;
    int rd = i.r_type.rd;
    
    regs[rd].u64[0] = instr.pc + 8;
    pc = regs[rs].u32[0];

	next_instr.is_delay_slot = true;
	branch_taken = true;

    if (can_disassemble) printf("jalr %s, %s\n", Reg(rs), Reg(rd));
}

void EmotionEngine::movn(Opcode i)
{
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;

    if (regs[rt].u64[0] != 0) regs[rd].u64[0] = regs[rs].u64[0];

    if (can_disassemble) printf("movn %s, %s, %s\n", Reg(rt), Reg(rd), Reg(rs));
}

void EmotionEngine::syscall(Opcode i)
{
	can_disassemble = false;
	//printf("syscall num %d\n", regs[3].u64[0]);
	exception(8, false);
}

void EmotionEngine::movz(Opcode i)
{
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;

    if (regs[rt].u64[0] == 0) regs[rd].u64[0] = regs[rs].u64[0];

    if (can_disassemble) printf("movz %s, %s, %s\n", Reg(rt), Reg(rd), Reg(rs));
}

void EmotionEngine::mfhi(Opcode i)
{
    int rd = i.r_type.rd;

    regs[rd].u64[0] = hi;

    if (can_disassemble) printf("mfhi %s\n", Reg(rd));
}

void EmotionEngine::mflo(Opcode i)
{
    int rd = i.r_type.rd;

    regs[rd].u64[0] = lo;

    if (can_disassemble) printf("mflo %s\n", Reg(rd));
}

void EmotionEngine::dsllv(Opcode i)
{
    int rs = i.r_type.rs;
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;

    uint64_t reg = regs[rt].u64[0];
    uint16_t sa = regs[rs].u32[0] & 0x3F;
    regs[rd].u64[0] = reg << sa;

    if (can_disassemble) printf("dsllv %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void EmotionEngine::dsrav(Opcode i)
{
    int rs = i.r_type.rs;
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;

    int64_t reg = (int64_t)regs[rt].u64[0];
    uint16_t sa = regs[rs].u32[0] & 0x3F;
    regs[rd].u64[0] = reg >> sa;

    if (can_disassemble) printf("dsrav %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void EmotionEngine::mult(Opcode i)
{
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;

    int64_t reg1 = (int64_t)regs[rs].u64[0];
    int64_t reg2 = (int64_t)regs[rt].u64[0];
    int64_t result = reg1 * reg2;

    regs[rd].u64[0] = lo = (int32_t)(result & 0xFFFFFFFF);
    hi = (int32_t)(result >> 32);

    if (can_disassemble) printf("mult %s, %s, %s (0x%08lx, 0x%08lx)\n", Reg(rd), Reg(rs), Reg(rt), hi, lo);
}

void EmotionEngine::div(Opcode i)
{
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;
    
    int32_t reg1 = regs[rs].u32[0];
    int32_t reg2 = regs[rt].u32[0];

    if (reg2 == 0)
    {
        hi = reg1;
        lo = reg1 >= 0 ? (int32_t)0xffffffff : 1;
    }
    else if (reg1 == (int32_t)0x80000000 && reg2 == -1)
    {
        hi = 0;
        lo = (int32_t)0x80000000;
    }
    else
    {
        hi = (int32_t)(reg1 % reg2);
        lo = (int32_t)(reg1 / reg2);
    }

    if (can_disassemble) printf("div %s, %s\n", Reg(rs), Reg(rt));
}

void EmotionEngine::divu(Opcode i)
{
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;

    if (regs[rt].u32[0] == 0)
    {
        hi = (int32_t)regs[rs].u32[0];
        lo = (int32_t)0xffffffff;
    }
    else
    {
        hi = (int32_t)(regs[rs].u32[0] % regs[rt].u32[0]);
        lo = (int32_t)(regs[rs].u32[0] / regs[rt].u32[0]);
    }

    if (can_disassemble) printf("divu %s (0x%08x), %s (0x%08x) (0x%08lx, 0x%08lx)\n", Reg(rs), regs[rs].u32[0], Reg(rt), regs[rt].u32[0], hi, lo);
}

void EmotionEngine::addu(Opcode i)
{
    int rt = i.r_type.rt;
    int rs = i.r_type.rs;
    int rd = i.r_type.rd;

    int32_t result = regs[rs].u64[0] + regs[rt].u64[0];
    regs[rd].u64[0] = result;

    if (can_disassemble) printf("addu %s, %s, %s\n", Reg(rd), Reg(rs), Reg(rt));
}

void EmotionEngine::subu(Opcode i)
{
    int rt = i.r_type.rt;
    int rs = i.r_type.rs;
    int rd = i.r_type.rd;

    int32_t reg1 = regs[rs].u64[0];
    int32_t reg2 = regs[rt].u64[0];
    regs[rd].u64[0] = reg1 - reg2;

    if (can_disassemble) printf("subu %s, %s, %s\n", Reg(rd), Reg(rs), Reg(rt));
}

void EmotionEngine::op_and(Opcode i)
{
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;

    regs[rd].u64[0] = regs[rt].u64[0] & regs[rs].u64[0];

    if (can_disassemble) printf("and %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void EmotionEngine::op_or(Opcode i)
{
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;

    regs[rd].u64[0] = regs[rt].u64[0] | regs[rs].u64[0];

    if (can_disassemble) printf("or %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void EmotionEngine::op_nor(Opcode i)
{
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;

    regs[rd].u64[0] = ~(regs[rt].u64[0] | regs[rs].u64[0]);

    if (can_disassemble) printf("nor %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));
}

void EmotionEngine::slt(Opcode i)
{
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;

    int64_t reg1 = regs[rs].u64[0];
    int64_t reg2 = regs[rt].u64[0];
    regs[rd].u64[0] = reg1 < reg2;
    
    if (can_disassemble) printf("slt %s, %s (0x%08lx), %s (0x%08lx)\n", Reg(rd), Reg(rs), reg1, Reg(rt), reg2);
}

void EmotionEngine::sltu(Opcode i)
{
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;

    regs[rd].u64[0] = regs[rs].u64[0] < regs[rt].u64[0];

    if (can_disassemble) printf("sltu %s, %s, %s\n", Reg(rd), Reg(rs), Reg(rt));
}

void EmotionEngine::daddu(Opcode i)
{
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;

    int64_t reg1 = regs[rs].u64[0];
    int64_t reg2 = regs[rt].u64[0];

    regs[rd].u64[0] = reg1 + reg2;

    if (can_disassemble) printf("daddu %s, %s, %s (0x%08lx)\n", Reg(rd), Reg(rs), Reg(rt), regs[rd].u64[0]);
}

void EmotionEngine::dsll(Opcode i)
{
    int sa = i.r_type.sa;
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;

    if (can_disassemble) printf("dsll %s, %s, %d\n", Reg(rd), Reg(rt), sa);

    regs[rd].u64[0] = regs[rt].u64[0] << sa;
}

void EmotionEngine::dsrl(Opcode i)
{
    int sa = i.r_type.sa;
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;

    if (can_disassemble) printf("dsrl %s, %s, %d\n", Reg(rd), Reg(rt), sa);

    regs[rd].u64[0] = regs[rt].u64[0] >> sa;
}

void EmotionEngine::dsll32(Opcode i)
{
    int sa = i.r_type.sa;
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;

    if (can_disassemble) printf("dsll32 %s, %s, %d\n", Reg(rd), Reg(rt), sa);

    regs[rd].u64[0] = regs[rt].u64[0] << (sa + 32);
}

void EmotionEngine::dsrl32(Opcode i)
{
    int sa = i.r_type.sa;
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;

    if (can_disassemble) printf("dsrl32 %s, %s, %d\n", Reg(rd), Reg(rt), sa);

    regs[rd].u64[0] = regs[rt].u64[0] >> (sa + 32);
}

void EmotionEngine::dsra32(Opcode i)
{
    int sa = i.r_type.sa;
    int rd = i.r_type.rd;
    int rt = i.r_type.rt;

    if (can_disassemble) printf("dsra32 %s, %s, %d\n", Reg(rd), Reg(rt), sa);

	int64_t reg = (int64_t)regs[rt].u64[0];
    regs[rd].u64[0] = reg >> (sa + 32);
}

void EmotionEngine::bltz(Opcode i)
{
    int32_t imm = (int16_t)i.i_type.imm;
	uint16_t rs = i.i_type.rs;

	int32_t offset = imm << 2;
	int64_t reg = (int64_t)regs[rs].u64[0];
	
	if (can_disassemble) printf("bltz %s, 0x%08x\n", Reg(rs), instr.pc + 4 + offset);

	if (reg < 0)
	{
		pc = instr.pc + 4 + offset;
		branch_taken = true;
	}

	next_instr.is_delay_slot = true;
}

void EmotionEngine::bgez(Opcode i)
{
    int32_t imm = (int16_t)i.i_type.imm;
	uint16_t rs = i.i_type.rs;

	int32_t offset = imm << 2;
	int64_t reg = (int64_t)regs[rs].u64[0];
	
	if (can_disassemble) printf("bgez %s, 0x%08x\n", Reg(rs), instr.pc + 4 + offset);
	
	if (reg >= 0)
	{
		pc = instr.pc + 4 + offset;
		branch_taken = true;
	}

	next_instr.is_delay_slot = true;
}

void EmotionEngine::bltzl(Opcode i)
{
	int32_t imm = (int16_t)i.i_type.imm;
	uint16_t rs = i.i_type.rs;

	int32_t offset = imm << 2;
	int64_t reg = (int64_t)regs[rs].u64[0];
	
	if (can_disassemble) printf("bltzl %s, 0x%08x\n", Reg(rs), instr.pc + 4 + offset);
	
	if (reg < 0)
	{
		pc = instr.pc + 4 + offset;
		branch_taken = true;
	}
	else
		fetch_next();

	next_instr.is_delay_slot = true;
}

void EmotionEngine::bgezl(Opcode i)
{
    int32_t imm = (int16_t)i.i_type.imm;
	uint16_t rs = i.i_type.rs;

	int32_t offset = imm << 2;
	int64_t reg = regs[rs].u64[0];
	
	if (can_disassemble) printf("bgezl %s, 0x%08x\n", Reg(rs), instr.pc + 4 + offset);

	if (reg >= 0)
	{
		pc = instr.pc + 4 + offset;
		branch_taken = true;
	}
	else
		fetch_next();

	next_instr.is_delay_slot = true;
}

void EmotionEngine::bgezal(Opcode i)
{
    int32_t imm = (int16_t)i.i_type.imm;
	uint16_t rs = i.i_type.rs;

	int32_t offset = imm << 2;
	int64_t reg = regs[rs].u64[0];
	
	if (can_disassemble) printf("bgezal %s, 0x%08x\n", Reg(rs), instr.pc + 4 + offset);

	if (reg >= 0)
	{
		regs[31].u64[0] = instr.pc + 8;
		pc = instr.pc + 4 + offset;
		branch_taken = true;
	}

	next_instr.is_delay_slot = true;
}

void EmotionEngine::bgezall(Opcode i)
{
    int32_t imm = (int16_t)i.i_type.imm;
	uint16_t rs = i.i_type.rs;

	int32_t offset = imm << 2;
	int64_t reg = regs[rs].u64[0];
	
	if (can_disassemble) printf("bgezall %s, 0x%08x\n", Reg(rs), instr.pc + 4 + offset);

	if (reg >= 0)
	{
		regs[31].u64[0] = instr.pc + 8;
		pc = instr.pc + 4 + offset;
		branch_taken = true;
	}
	else
		fetch_next();

	next_instr.is_delay_slot = true;
}


void EmotionEngine::mflo1(Opcode i)
{
    int rd = i.r_type.rd;

    regs[rd].u64[0] = lo1;

    if (can_disassemble) printf("mflo1 %s\n", Reg(rd));
}

void EmotionEngine::mult1(Opcode i)
{
    int rs = i.r_type.rs;
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;

    int64_t reg1 = (int64_t)regs[rs].u32[0];
    int64_t reg2 = (int64_t)regs[rt].u32[0];
    int64_t result = reg1 * reg2;

    regs[rd].u64[0] = lo1 = (int32_t)(result & 0xFFFFFFFF);
    hi1 = (int32_t)(result >> 32);

    if (can_disassemble) printf("mult1 %s, %s, %s \n", Reg(rd), Reg(rs), Reg(rt));
}

void EmotionEngine::divu1(Opcode i)
{
    int rt = i.i_type.rt;
    int rs = i.i_type.rs;

    if (regs[rt].u32[0] != 0)
    {
        lo1 = (int64_t)(int32_t)(regs[rs].u32[0] / regs[rt].u32[0]);
        hi1 = (int64_t)(int32_t)(regs[rs].u32[0] % regs[rt].u32[0]);
        if (can_disassemble) printf("divu1 %s, %s\n", Reg(rs), Reg(rt));
    }
    else
    {
        printf("DIVU1: Division by zero!\n");
        Application::Exit(1);
    }
}

void EmotionEngine::padduw(Opcode i)
{
	uint64_t reg1 = (i.full >> 21) & 0x1F;
	uint64_t reg2 = (i.full >> 16) & 0x1F;
	uint64_t dest = (i.full >> 11) & 0x1F;

	for (int i = 0; i < 4; i++)
	{
		uint64_t result = regs[reg1].u32[i];
		result += regs[reg2].u32[i];
		if (result > 0xFFFFFFFF)
			result -= 0xFFFFFFFF;
		regs[dest].u32[i] = (uint32_t)result;
	}

	if (can_disassemble)
		printf("padduw %s, %s, %s\n", Reg(dest), Reg(reg1), Reg(reg2));
}


void EmotionEngine::por(Opcode i)
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;
    int rs = i.r_type.rs;

    if (can_disassemble) printf("por %s, %s, %s\n", Reg(rd), Reg(rt), Reg(rs));

    regs[rd].u64[0] = regs[rs].u64[0] | regs[rt].u64[0];
    regs[rd].u64[1] = regs[rs].u64[1] | regs[rt].u64[1];
}


void EmotionEngine::mfc1(Opcode i)
{
    regs[i.r_type.rt].u64[0] = cop1.regs.u[i.r_type.rd];
    if (can_disassemble) printf("mfc1 %s, f%d\n", Reg(i.r_type.rt), i.r_type.rd);
}

void EmotionEngine::mtc1(Opcode i)
{
    cop1.regs.u[i.r_type.rd] = regs[i.r_type.rt].u32[0];
    if (can_disassemble) printf("mtc1 %s, f%d\n", Reg(i.r_type.rt), i.r_type.rd);
}

void EmotionEngine::ctc1(Opcode i)
{
	int reg = i.i_type.rt;
	int cop_reg = i.r_type.rd;

	uint32_t value = regs[reg].u32[0];

	auto& control = cop1.control;

	if (cop_reg == 31)
	{
		control.su = value & (1 << 3);
        control.so = value & (1 << 4);
        control.sd = value & (1 << 5);
        control.si = value & (1 << 6);
        control.u = value & (1 << 14);
        control.o = value & (1 << 15);
        control.d = value & (1 << 16);
        control.i = value & (1 << 17);
        control.condition = value & (1 << 23);
	}

	if (can_disassemble) printf("ctc1 %s, r%d\n", Reg(reg), cop_reg);
}


void EmotionEngine::bc(Opcode i)
{
	const static bool likely[] = {false, false, true, true};
	const static bool op_true[] = {false, true, false, true};
	int32_t offset = ((int16_t)i.i_type.imm) << 2;
	uint8_t op = i.i_type.rt;

	bool test_true = op_true[op];
	bool l = likely[op];

	bool passed = false;
	if (test_true)
		passed = cop1.control.condition;
	else
		passed = !cop1.control.condition;
	
	if (passed)
	{
		pc = instr.pc + 4 + offset;
	}
	else if (l)
		fetch_next();
}


void EmotionEngine::adda(Opcode i)
{
    int rt = i.r_type.rt;
    int rd = i.r_type.rd;

    float op1 = convert(cop1.regs.u[rt]);
    float op2 = convert(cop1.regs.u[rd]);

    cop1.accumulator.f = op1 + op2;

	check_overflow_cop1(cop1.accumulator.u, true);
	check_underflow_cop1(cop1.accumulator.u, true);
    if (can_disassemble) printf("adda.s f%d, f%d\n", rt, rd);
}


void EmotionEngine::qmfc2(Opcode i)
{
	int dest = i.r_type.rt;
	int cop_reg = i.r_type.rd;

	for (int i = 0; i < 4; i++)
	{
		regs[dest].u32[i] = vu0->GetGprU(cop_reg, i);
	}

	if (can_disassemble) printf("qmfc2 %s, v%d\n", Reg(dest), cop_reg);
}

void EmotionEngine::qmtc2(Opcode i)
{
	int source = i.r_type.rt;
	int cop_reg = i.r_type.rd;

	for (int i = 0; i < 4; i++)
	{
		uint32_t bark = regs[source].u32[i];
		vu0->SetGprU(cop_reg, i, bark);
	}

	if (can_disassemble) printf("qmtc2 %s, v%d\n", Reg(source), cop_reg);
}

void EmotionEngine::cfc2(Opcode i)
{
	uint16_t id = i.r_type.rd;
	uint16_t rt = i.r_type.rt;

	regs[rt].u64[0] = (int32_t)vu0->cfc(id);

	if (can_disassemble) printf("cfc2 %s, c%ld\n", Reg(rt), id);
}

void EmotionEngine::ctc2(Opcode i)
{
	uint64_t id = i.r_type.rt;
	uint16_t rd = i.r_type.rd;

	vu0->ctc(id, regs[rd].u32[0]);

	if (can_disassemble) printf("ctc2 %s, c%ld\n", Reg(rd), id);
}