// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include <emu/memory/Bus.h>
#include <app/Application.h>
#include <emu/cpu/iop/cpu.h>

#include <cstring>
#include "cpu.h"

uint32_t exception_addr[2] = { 0x80000080, 0xBFC00180 };

void CPU::exception(Exception cause, uint32_t cop)
{
	uint32_t mode = Cop0.status.value;
	Cop0.status.value &= ~(uint32_t)0x3F;
	Cop0.status.value |= (mode << 2) & 0x3F;

	Cop0.cause.excode = (uint32_t)cause;
	Cop0.cause.CE = cop;

	bool is_delay_slot = i.is_delay_slot;
	bool branch_taken = i.branch_taken;
	if (cause == Exception::Interrupt)
	{
		Cop0.epc = next_instr.pc;

		is_delay_slot = next_instr.is_delay_slot;
		branch_taken = next_instr.branch_taken;
	}
	else
	{
		Cop0.epc = i.pc;
	}

	if (is_delay_slot)
	{
		Cop0.epc -= 4;

		Cop0.cause.BD = true;
		Cop0.TAR = next_instr.pc;

		if (branch_taken)
		{
			Cop0.cause.BT = true;
		}
	}

	pc = exception_addr[Cop0.status.BEV];

	direct_jump();
}

void CPU::Reset()
{
    memset(regs, 0, sizeof(regs));
    Cop0 = {};

    Cop0.regs[15] = 0x1f;

    pc = 0xbfc00000;

	direct_jump();

	console.open("iop_console.txt");
}

CPU::~CPU()
{
	console.flush();
	console.close();
}

void CPU::direct_jump()
{
	next_instr = {};
	next_instr.full = Bus::iop_read<uint32_t>(pc);
	next_instr.pc = pc;
	pc += 4;
}

void CPU::handle_load_delay()
{
	if (delayed_memory_load.reg != memory_load.reg)
	{
		regs[memory_load.reg] = memory_load.data;
	}

	memory_load = delayed_memory_load;
	delayed_memory_load.reg = 0;

	regs[write_back.reg] = write_back.data;
	write_back.reg = 0;
	regs[0] = 0;
}

void CPU::branch(std::string op)
{
	int32_t imm = (int16_t)i.i_type.imm;

	next_instr.branch_taken = true;
	pc = next_instr.pc + (imm << 2);

	if (can_disassemble) printf("%s, 0x%08x\n", op.c_str(), pc);
}

void CPU::branch()
{
	int32_t imm = (int16_t)i.i_type.imm;

	next_instr.branch_taken = true;
	pc = next_instr.pc + (imm << 2);
}

void CPU::load(uint32_t regN, uint32_t value)
{
	delayed_memory_load.reg = regN;
	delayed_memory_load.data = value;
}

void CPU::Clock(uint64_t cycles)
{
    for (uint64_t cycle = 0; cycle < cycles; cycle++)
    {	
		i = next_instr;

		if (i.pc == 0x12C48 || i.pc == 0x1420C || i.pc == 0x1230C)
		{
			uint32_t ptr = regs[5];
			uint32_t text_size = regs[6];
			while (text_size)
			{
				auto c = (char)Bus::iop_read<uint8_t>(ptr & 0x1FFFFF);
				console << c;
				console.flush();

				ptr++;
				text_size--;
			}
		}
		
		if (pc & 0x3)
		{
			printf("Error: Unaligned address at 0x%08x\n", pc);
			exit(1);
		}

		if (can_disassemble)
			printf("0x%08x: ", i.pc);

		direct_jump();

		switch (i.opcode)
        {
        case 0b000000: op_special(); break;
        case 0b000001: op_bcond(); break;
        case 0b001111: op_lui(); break;
        case 0b001101: op_ori(); break;
		case 0b001110: op_xori(); break;
        case 0b101011: op_sw(); break;
        case 0b001001: op_addiu(); break;
        case 0b001000: op_addi(); break;
        case 0b000010: op_j(); break;
        case 0b010000: op_cop0(); break;
        case 0b100011: op_lw(); break;
        case 0b000101: op_bne(); break;
        case 0b101001: op_sh(); break;
        case 0b000011: op_jal(); break;
        case 0b001100: op_andi(); break;
        case 0b101000: op_sb(); break;
        case 0b100000: op_lb(); break;
        case 0b000100: op_beq(); break;
        case 0b000111: op_bgtz(); break;
        case 0b000110: op_blez(); break;
        case 0b100100: op_lbu(); break;
        case 0b001010: op_slti(); break;
        case 0b001011: op_sltiu(); break;
        case 0b100101: op_lhu(); break;
        case 0b100001: op_lh(); break;
		case 0b100010: op_lwl(); break;
		case 0b100110: op_lwr(); break;
		case 0b101010: op_swl(); break;
		case 0b101110: op_swr(); break;
		case 0x12: op_cop2(); break;
        default:
			printf("[emu/IOP]: Unknown instruction 0x%02x\n", i.opcode);
			exit(1);
        }

        /* Apply pending load delays. */
        handle_load_delay();
    }

	if (i.opcode != 0b100010)
	{
		last_instruction_was_lwl = false;
	}
	if (i.opcode != 0b100110)
	{
		last_instruction_was_lwr = false;
	}

	if (IntPending())
		exception(Exception::Interrupt);
}

void CPU::Dump()
{
    for (int i = 0; i < 32; i++)
        printf("[emu/IOP]: %s: %s\t->\t0x%08x\n", __FUNCTION__, Reg(i), regs[i]);
    printf("[emu/IOP]: %s: pc\t->\t0x%08x\n", __FUNCTION__, pc - 4);
	printf("[emu/IOP]: COP0_Cause: 0x%08x\n", Cop0.cause.value);
	printf("[emu/IOP]: COP0_Status: 0x%08x\n", Cop0.status.value);
}

bool CPU::IntPending()
{	
	bool pending = Bus::I_CTRL && (Bus::I_STAT & Bus::I_MASK);
	Cop0.cause.IP = (Cop0.cause.IP & ~0x4) | (pending << 2);

	bool enabled = Cop0.status.IEc && (Cop0.status.Im & Cop0.cause.IP);

	return pending && enabled;
}

bool CPU::CanDisassemble()
{
    return can_disassemble;
}

static CPU iop;

void IOP_MANAGEMENT::Clock(uint64_t cycles)
{
	iop.Clock(cycles);
}

void IOP_MANAGEMENT::Reset()
{
	iop.Reset();
}

void IOP_MANAGEMENT::Dump()
{
    iop.Dump();
}

bool IOP_MANAGEMENT::CanDisassemble()
{
    return iop.CanDisassemble();
}
