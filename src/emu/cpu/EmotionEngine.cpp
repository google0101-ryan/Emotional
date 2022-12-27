#include "app/Application.h"
#include "util/uint128.h"
#include <bits/stdint-uintn.h>
#include <cstring>
#include <emu/cpu/EmotionEngine.h>
#include <emu/cpu/opcode.h>
#include <assert.h>
#include "EmotionEngine.h"

union COP0Cause
{
    uint32_t value;
    struct
    {
        uint32_t : 2;
        uint32_t exccode : 5;
        uint32_t : 3;
        uint32_t ip0_pending : 1;
        uint32_t ip1_pending : 1;
        uint32_t siop : 1;
        uint32_t : 2;
        uint32_t timer_ip_pending : 1;
        uint32_t exc2 : 3;
        uint32_t : 9;
        uint32_t ce : 2;
        uint32_t bd2 : 1;
        uint32_t bd : 1;
    };
};

void EmotionEngine::exception(int type, bool log)
{
	static uint32_t exception_addr[2] = { 0x80000000, 0xBFC00200 };

	if (log)
	{
		printf("Exception of type %d\n", type);
	}

	COP0Cause cause;
	cause.value = cop0_regs[13];
	uint32_t status = cop0_regs[12];
	bool edi = (status >> 17) & 1;
	bool exl = (status >> 1) & 1;
	bool erl = (status >> 2) & 1;
	int ksu = (status >> 3) & 2;

	uint32_t vector = 0x180;
	cause.exccode = type;
	if (!exl)
	{
		cop0_regs[14] = instr.pc - 4 * instr.is_delay_slot;
		cause.bd = instr.is_delay_slot;

		switch (type)
		{
		case 0:
			vector = 0x200;
			break;
		default:
			vector = 0x180;
			break;
		}

		cop0_regs[12] |= (1 << 1);
	}

	pc = exception_addr[status & (1 << 22)] + vector;

	cop0_regs[13] = cause.value;

	fetch_next();
}

EmotionEngine::EmotionEngine(Bus *bus, VectorUnit *vu0)
	: bus(bus),
	  vu0(vu0)
{
    memset(regs, 0, sizeof(regs));
    pc = 0xBFC00000;
    
    next_instr = {};
    next_instr.full = bus->read<uint32_t>(pc);
    next_instr.pc = pc;
	pc += 4;

    cop0_regs[15] = 0x2E20;
}

void EmotionEngine::Clock(int cycles)
{
    int cycles_to_execute = cycles;
    for (int cycle = 0; cycle < cycles; cycle++)
    {
		//if (next_instr.pc == 0x82000) can_disassemble = true;
        instr = next_instr;

		fetch_next();

		branch_taken = false;

		// if ((pc & 0xffff) == 0x10b0)
		// 	can_disassemble = true;

		if (pc & 0x3)
		{
			printf("Unaligned PC: 0x%08x\n", pc);
			Application::Exit(1);
		}

		if (can_disassemble)
			printf("0x%08x: ", instr.full);

        switch (instr.r_type.opcode)
        {
		case 0x00:
			switch (instr.r_type.func)
			{
			case 0x00:
				sll(instr);
				break;
			case 0x02:
				srl(instr);
				break;
			case 0x03:
				sra(instr);
				break;
			case 0x04:
				sllv(instr);
				break;
			case 0x06:
				srlv(instr);
				break;
			case 0x07:
				srav(instr);
				break;
			case 0x08:
				jr(instr);
				break;
			case 0x09:
				jalr(instr);
				break;
			case 0x0A:
				movz(instr);
				break;
			case 0x0B:
				movn(instr);
				break;
			case 0x0C:
				syscall(instr);
				break;
			case 0x0f:
				// Sync
				if (can_disassemble)
					printf("sync\n");
				break;
			case 0x10:
				mfhi(instr);
				break;
			case 0x12:
				mflo(instr);
				break;
			case 0x14:
				dsllv(instr);
				break;
			case 0x17:
				dsrav(instr);
				break;
			case 0x18:
				mult(instr);
				break;
			case 0x1A:
				div(instr);
				break;
			case 0x1b:
				divu(instr);
				break;
			case 0x21:
				addu(instr);
				break;
			case 0x23:
				subu(instr);
				break;
			case 0x24:
				op_and(instr);
				break;
			case 0x25:
				op_or(instr);
				break;
			case 0x27:
				op_nor(instr);
				break;
			case 0x2a:
				slt(instr);
				break;
			case 0x2b:
				sltu(instr);
				break;
			case 0x2d:
				daddu(instr);
				break;
			case 0x38:
				dsll(instr);
				break;
			case 0x3a:
				dsrl(instr);
				break;
			case 0x3c:
				dsll32(instr);
				break;
			case 0x3e:
				dsrl32(instr);
				break;
			case 0x3f:
				dsra32(instr);
				break;
			default:
				printf("[emu/CPU]: %s: Uknown special instruction 0x%02x (0x%08x)\n", __FUNCTION__, instr.r_type.func, instr.full);
				Application::Exit(1);
			}
			break;
		case 0x01:
			switch (instr.i_type.rt)
			{
			case 0x00:
				bltz(instr);
				break;
			case 0x01:
				bgez(instr);
				break;
			case 0x02:
				bltzl(instr);
				break;
			case 0x03:
				bgezl(instr);
				break;
			case 0x11:
				bgezal(instr);
				break;
			case 0x13:
				bgezall(instr);
				break;
			default:
				printf("[emu/CPU]: %s: Uknown regimm instruction 0x%02x (0x%08x)\n", __FUNCTION__, instr.r_type.rt, instr.full);
				Application::Exit(1);
			}
			break;
		case 0x02:
			j(instr);
			break;
		case 0x03:
			jal(instr);
			break;
		case 0x04:
			beq(instr);
			break;
		case 0x05:
			bne(instr);
			break;
		case 0x06:
			blez(instr);
			break;
		case 0x07:
			bgtz(instr);
			break;
		case 0x09:
			addiu(instr);
			break;
		case 0x0a:
			slti(instr);
			break;
		case 0x0b:
			sltiu(instr);
			break;
		case 0x0c:
			andi(instr);
			break;
		case 0x0d:
			ori(instr);
			break;
		case 0x0e:
			xori(instr);
			break;
		case 0x0f:
			lui(instr);
			break;
		case 0x10:
			switch (instr.r_type.rs)
			{
			case 0x00:
				mfc0(instr);
				break;
			case 0x04:
				mtc0(instr);
				break;
			case 0x10:
				switch (instr.r_type.func)
				{
				case 0x01:
				case 0x02:
				case 0x06:
					break;
				case 0x18:
					eret(instr);
					break;
				case 0x38:
					ei(instr);
					break;
				case 0x39:
					di(instr);
					break;
				default:
					printf("[emu/CPU]: %s: Uknown tlb instruction 0x%02x (0x%08x)\n", __FUNCTION__, instr.r_type.func, instr.full);
					Application::Exit(1);
				}
				break;
			default:
				printf("[emu/CPU]: %s: Uknown cop0 instruction 0x%02x (0x%08x)\n", __FUNCTION__, instr.r_type.rs, instr.full);
				Application::Exit(1);
			}
			break;
		case 0x11:
		{
			switch (instr.r_type.rs)
			{
			case 0x04:
				mtc1(instr);
				break;
			case 0x06:
				ctc1(instr);
				break;
			case 0x10:
				bc(instr);
				break;
			default:
				printf("[emu/CPU]: %s: Unknown cop1 instruction 0x%02x (0x%08x)\n", __FUNCTION__, instr.r_type.rs, instr.full);
				Application::Exit(1);
			}
			break;
		}
		case 0x12:
			switch (instr.r_type.rs)
			{
			case 0x01:
				qmfc2(instr);
				break;
			case 0x02:
				cfc2(instr);
				break;
			case 0x05:
				qmtc2(instr);
				break;
			case 0x06:
				ctc2(instr);
				break;
			case 0x10 ... 0x1F:
				vu0->special1(instr);
				break;
			default:
				printf("[emu/CPU]: %s: Uknown cop2 instruction 0x%02x (0x%08x)\n", __FUNCTION__, instr.r_type.rs, instr.full);
				Application::Exit(1);
			}
			break;
		case 0x14:
			beql(instr);
			break;
		case 0x15:
			bnel(instr);
			break;
		case 0x19:
			daddiu(instr);
			break;
		case 0x1a:
			ldl(instr);
			break;
		case 0x1b:
			ldr(instr);
			break;
		case 0x1c:
		{
			switch (instr.r_type.func)
			{
			case 0x12:
				mflo1(instr);
				break;
			case 0x18:
				mult1(instr);
				break;
			case 0x1b:
				divu1(instr);
				break;
			case 0x28:
			{
				switch (instr.r_type.sa)
				{
				case 0x10:
					padduw(instr);
					break;
				default:
					printf("[emu/CPU]: %s: Unknown mmi1 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.r_type.sa);
					Application::Exit(1);
				}
				break;
			}
			case 0x29:
			{
				switch (instr.r_type.sa)
				{
				case 0x12:
					por(instr);
					break;
				default:
					printf("[emu/CPU]: %s: Unknown mmi3 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.r_type.sa);
					Application::Exit(1);
				}
				break;
			}
			default:
				printf("[emu/CPU]: %s: Unknown mmi 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.r_type.func);
				Application::Exit(1);
			}
			break;
		}
		case 0x1e:
			lq(instr);
			break;
		case 0x1f:
			sq(instr);
			break;
		case 0x20:
			lb(instr);
			break;
		case 0x21:
			lh(instr);
			break;
		case 0x23:
			lw(instr);
			break;
		case 0x24:
			lbu(instr);
			break;
		case 0x25:
			lhu(instr);
			break;
		case 0x27:
			lwu(instr);
			break;
		case 0x28:
			sb(instr);
			break;
		case 0x29:
			sh(instr);
			break;
		case 0x2b:
			sw(instr);
			break;
		case 0x2c:
			sdl(instr);
			break;
		case 0x2d:
			sdr(instr);
			break;
		case 0x2f:
			// Cache
			if (can_disassemble)
				printf("cache\n");
			break;
		case 0x37:
			ld(instr);
			break;
		case 0x39:
			swc1(instr);
			break;
		case 0x3f:
			sd(instr);
			break;
        default:
            printf("[emu/CPU]: %s: Unknown instruction 0x%08x (0x%02x)\n", __FUNCTION__, instr.full, instr.r_type.opcode);
            Application::Exit(1);
        }
        
        cycles_to_execute--;
    }
	
    cop0_regs[9] += cycles + std::abs(cycles_to_execute);

    regs[0].u64[0] = regs[0].u64[1] = 0;

	if (bus->read<uint32_t>(0x1000f000) & bus->read<uint32_t>(0x1000f010))
	{
		printf("Need to trigger interrupt here\n");
		exit(1);
	}
}

void EmotionEngine::Dump()
{
    for (int i = 0; i < 32; i++)
        printf("[emu/CPU]: %s: %s\t->\t%s\n", __FUNCTION__, Reg(i), print_128(regs[i]));
    for (int i = 0; i < 32; i++)
        printf("[emu/CPU]: %s: f%d\t->\t%0.2f\n", __FUNCTION__, i, cop1.regs.f[i]);
    printf("[emu/CPU]: %s: pc\t->\t0x%08x\n", __FUNCTION__, pc-4);
    printf("[emu/CPU]: %s: hi\t->\t0x%08lx\n", __FUNCTION__, hi);
    printf("[emu/CPU]: %s: lo\t->\t0x%08lx\n", __FUNCTION__, lo);
    printf("[emu/CPU]: %s: hi1\t->\t0x%08lx\n", __FUNCTION__, hi1);
    printf("[emu/CPU]: %s: lo1\t->\t0x%08lx\n", __FUNCTION__, lo1);
}