#include "app/Application.h"
#include "util/uint128.h"
#include <bits/stdint-uintn.h>
#include <cstring>
#include <map>
#include <emu/cpu/EmotionEngine.h>
#include <emu/cpu/opcode.h>
#include <assert.h>
#include "EmotionEngine.h"

void EmotionEngine::exception(int type, bool log)
{
	static uint32_t exception_addr[2] = { 0x80000000, 0xBFC00200 };

	if (log)
	{
		printf("Exception of type %d\n", type);
	}

	COP0Cause cause;
	cause.value = cop0.cop0_regs[13];
	COP0Status status;
	status.value = cop0.cop0_regs[12];

	uint32_t vector = 0x180;
	cause.exccode = type;
	if (!status.exl)
	{
		cop0.cop0_regs[14] = instr.pc - 4 * instr.is_delay_slot;
		cause.bd = instr.is_delay_slot;

		switch (type)
		{
		case 0:
			//can_disassemble = true;
			vector = 0x200;
			break;
		default:
			vector = 0x180;
			break;
		}

		status.exl = true;
	}

	pc = exception_addr[status.bev] + vector;

	cop0.cop0_regs[13] = cause.value;
	cop0.cop0_regs[12] = status.value;

	fetch_next();

	tlb_map = cop0.get_vtlb_map();
}

void EmotionEngine::HandleSifSetDma()
{
	struct SifDmaTransfer
	{
		uint32_t source;
		uint32_t dest;
		int size;
		int attr;
	};

	typedef struct t_SifCmdHeader
	{
		/** Packet size. Min: 1x16 (header only), max: 7*16 */
		uint32_t psize : 8;
		/** Payload size */
		uint32_t dsize : 24;
		/** Destination address for payload. Can be NULL if there is no payload. */
		uint32_t dest;
		/** Function number of function to call. */
		int cid;
		/** Can be freely used. */
		uint32_t opt;
	} SifCmdHeader_t;

	struct SifRpcBindPkt
	{
		SifCmdHeader_t sifcmd;
		int rec_id;
		uint32_t pkt_addr;
		int rpc_id;
		uint32_t client;
		uint32_t sid;
	};

	uint32_t addr = Bus::Translate(regs[4].u32[0]);
	
	SifDmaTransfer* dmat = (SifDmaTransfer*)&bus->grab_ee_ram()[addr];
	addr = Bus::Translate(dmat->source);
	SifCmdHeader_t* pkt = (SifCmdHeader_t*)&bus->grab_ee_ram()[addr];
	SifRpcBindPkt* bind = (SifRpcBindPkt*)pkt;

	std::map<uint32_t, std::string> servers =
	{
		{0x80000001, "FILEIO"},
		{0x80000003, "IOP Heap Allocation"},
		{0x80000006, "LOADFILE"},
		{0x80000100, "PADMAN"},
		{0x80000101, "PADMAN"},
		{0x80000400, "MCSERV"},
		{0x80000592, "CDVDFSV Init"},
		{0x80000593, "CDVDFSV S command"},
		{0x80000595, "CDVDFSV N command"},
		{0x80000597, "CDVDFSV Search file"},
		{0x8000059A, "CDVDFSV Disk Ready"},
	};

	if (pkt->cid == 0x80000009)
	{
		printf("Binding RPC server %s\n", servers[bind->sid].c_str());
	}
}

EmotionEngine::EmotionEngine(Bus *bus, VectorUnit *vu0)
	: bus(bus),
	  vu0(vu0),
	  cop0(bus->grab_ee_ram(), bus->grab_ee_bios(), bus->grab_ee_spr())
{
    memset(regs, 0, sizeof(regs));
    pc = 0xBFC00000;

	cop0.init_tlb();
	tlb_map = cop0.get_vtlb_map();
    
    next_instr = {};
    next_instr.full = bus->read<uint32_t>(pc);
    next_instr.pc = pc;
	pc += 4;

    cop0.cop0_regs[15] = 0x2E20;

	bus->AttachCPU(this);
}

bool EmotionEngine::int_pending()
{
	COP0Cause cause;
	cause.value = cop0.cop0_regs[13];

    COP0Status status;
	status.value = cop0.cop0_regs[12];

	bool int_enabled = status.eie && status.ie && !status.erl && !status.exl;

	bool pending = (cause.ip0_pending && status.im0) || (cause.ip1_pending && status.im1) || (cause.timer_ip_pending && status.im7);

	return int_enabled && pending;
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
			printf("0x%08x (0x%08x): ", instr.full, instr.pc);

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
			case 0x11:
				mthi(instr);
				break;
			case 0x12:
				mflo(instr);
				break;
			case 0x13:
				mtlo(instr);
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
			case 0x19:
				multu(instr);
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
			case 0x26:
				op_xor(instr);
				break;
			case 0x27:
				op_nor(instr);
				break;
			case 0x28:
				mfsa(instr);
				break;
			case 0x29:
				mtsa(instr);
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
			case 0x2f:
				dsubu(instr);
				break;
			case 0x38:
				dsll(instr);
				break;
			case 0x3a:
				dsrl(instr);
				break;
			case 0x3b:
				dsra(instr);
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
				case 0x02:
					tlbwi(instr);
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
			case 0x02:
				cfc1(instr);
				break;
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
		case 0x16:
			blezl(instr);
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
			case 0x04:
				plzcw(instr);
				break;
			case 0x08:
				paddb(instr);
				break;
			case 0x09:
				psubb(instr);
				break;
			case 0x10:
				mfhi1(instr);
				break;
			case 0x11:
				mthi1(instr);
				break;
			case 0x12:
				mflo1(instr);
				break;
			case 0x13:
				mtlo1(instr);
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
				case 0x09:
					pmtlo(instr);
					break;
				case 0x0E:
					pcpyud(instr);
					break;
				case 0x12:
					por(instr);
					break;
				case 0x13:
					pnor(instr);
					break;
				case 0x1B:
					pcpyh(instr);
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
		case 0x22:
			lwl(instr);
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
		case 0x26:
			lwr(instr);
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
		case 0x2a:
			swl(instr);
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
		case 0x2e:
			swr(instr);
			break;
		case 0x2f:
			// Cache
			if (can_disassemble)
				printf("cache\n");
			break;
		case 0x31:
			lwc1(instr);
			break;
		case 0x37:
			ld(instr);
			break;
		case 0x39:
			swc1(instr);
			break;
		case 0x3e:
			sqc2(instr);
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
	
    cop0.cop0_regs[9] += cycles + std::abs(cycles_to_execute);

    regs[0].u64[0] = regs[0].u64[1] = 0;

	if (int_pending())
	{
		COP0Cause cause;
		cause.value = cop0.cop0_regs[13];
		printf("Servicing interrupt (0x%08x)\n", instr.pc);
		exception(0, false);
	}
}

void EmotionEngine::Dump()
{
    for (int i = 0; i < 32; i++)
        printf("[emu/CPU]: %s: %s\t->\t%s\n", __FUNCTION__, Reg(i), print_128(regs[i]));
    for (int i = 0; i < 32; i++)
        printf("[emu/CPU]: %s: f%d\t->\t%0.2f\n", __FUNCTION__, i, cop1.regs.f[i]);
    printf("[emu/CPU]: %s: pc\t->\t0x%08x\n", __FUNCTION__, instr.pc);
    printf("[emu/CPU]: %s: hi\t->\t0x%08lx\n", __FUNCTION__, hi);
    printf("[emu/CPU]: %s: lo\t->\t0x%08lx\n", __FUNCTION__, lo);
    printf("[emu/CPU]: %s: hi1\t->\t0x%08lx\n", __FUNCTION__, hi1);
    printf("[emu/CPU]: %s: lo1\t->\t0x%08lx\n", __FUNCTION__, lo1);
}