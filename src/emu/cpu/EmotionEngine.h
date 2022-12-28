#pragma once

#include "emu/Bus.h"
#include "emu/cpu/opcode.h"
#include "util/uint128.h"
#include <bits/stdint-uintn.h>
#include <emu/cpu/ee/vu.h>

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

union COP0Status
{
    uint32_t value;
    struct
    {
        uint32_t ie : 1; /* Interrupt Enable */
        uint32_t exl : 1; /* Exception Level */
        uint32_t erl : 1; /* Error Level */
        uint32_t ksu : 2; /* Kernel/Supervisor/User Mode bits */
        uint32_t : 5;
        uint32_t im0 : 1; /* Int[1:0] signals */
        uint32_t im1 : 1;
        uint32_t bem : 1; /* Bus Error Mask */
        uint32_t : 2;
        uint32_t im7 : 1; /* Internal timer interrupt  */
        uint32_t eie : 1; /* Enable IE */
        uint32_t edi : 1; /* EI/DI instruction Enable */
        uint32_t ch : 1; /* Cache Hit */
        uint32_t : 3;
        uint32_t bev : 1; /* Location of TLB refill */
        uint32_t dev : 1; /* Location of Performance counter */
        uint32_t : 2;
        uint32_t fr : 1; /* Additional floating point registers */
        uint32_t : 1;
        uint32_t cu : 4; /* Usability of each of the four coprocessors */
    };
};

class EmotionEngine
{
private:
    Bus* bus;
    VectorUnit* vu0;

    uint128_t regs[32];
    uint32_t pc;
    uint64_t hi, lo;
    uint64_t hi1, lo1;
	uint32_t sa;

    Opcode next_instr, instr;

    void fetch_next()
    {
        next_instr = {};
        next_instr.full = bus->read<uint32_t>(pc);
        next_instr.pc = pc;
		next_instr.is_delay_slot = false;
        pc += 4;
    }

    struct COP1
    {
        union
        {
            float f[32] = {0.0f};
            uint32_t u[32];
			int32_t s[32];
        } regs = {{0.0f}};

        union
        {
            float f;
            uint32_t u;
            int32_t s;
        } accumulator = {0.f};

		struct COP1_CONTROL
		{
			bool su;
			bool so;
			bool sd;
			bool si;
			bool u;
			bool o;
			bool d;
			bool i;
			bool condition;
		} control = {false, false, false, 
					false, false, false, 
					false, false, false};
    } cop1;

	struct COP0
	{
	private:
		enum CACHE_MODE
		{
			UNCACHED = 2,
			CACHED = 3,
			UCAB = 7,
			SPR = 8
		};

		struct TLB_Entry
		{
			bool valid[2];
			bool dirty[2];
			uint8_t cache_mode[2];
			uint32_t pfn[2];
			bool is_spr;
			bool global;
			uint8_t asid;
			 uint32_t vpn2;
			uint32_t page_size;
			uint32_t page_mask;

			uint8_t page_shift;
		};

		struct VTLB_Info
		{
			uint8_t cache_mode;
			bool modified = false;
		};

		uint8_t* ram;
		uint8_t* bios;
		uint8_t* spr;

		uint8_t** kernel_vtlb;
		uint8_t** sup_vtlb;
		uint8_t** user_vtlb;

		VTLB_Info* vtlb_info;

		void unmap_tlb(TLB_Entry* entry);
		void map_tlb(TLB_Entry* entry);

		uint8_t* get_mem_pointer(uint32_t paddr);
	public:
    	uint32_t cop0_regs[32];
		COP0(uint8_t* RDRAM, uint8_t* BIOS, uint8_t* SPR);
		TLB_Entry tlb[48];
		uint32_t PCCR, PCR0, PCR1;

		void init_tlb();
		uint8_t** get_vtlb_map();

		void set_tlb_modified(size_t page);

		void set_tlb(int index);
	} cop0;

	uint8_t** tlb_map;

	void check_overflow_cop1(uint32_t& dest, bool set_flags)
	{
		if ((dest & ~0x80000000) == 0x7F800000)
		{
			printf("[FPU] Overflow Dest = %x\n", dest);
			dest = (dest & 0x80000000) | 0x7F7FFFFF;

			if (set_flags)
			{
				cop1.control.so = true;
				cop1.control.o = true;
			}
		}
		else
		{
			if (set_flags)
				cop1.control.o = false;
		}
	}

	void check_underflow_cop1(uint32_t& dest, bool set_flags)
	{
		auto& control = cop1.control;
		if ((dest & 0x7F800000) == 0 && (dest & 0x7FFFFF) != 0)
		{
			printf("[FPU] Underflow Dest = %x\n", dest);
			dest &= 0x80000000;
			
			if (set_flags)
			{
				control.su = true;
				control.u = true;
			}
		}
		else
		{
			if (set_flags)
				control.u = false;
		}
	}

    struct CacheTag
    {
        bool valid = false;
        bool dirty = false;
        bool lrf = false;
        uint32_t page = 0;
    };

    bool singleStep = false;
	bool can_disassemble = false;

    void j(Opcode i); // 0x02
    void jal(Opcode i); // 0x03
    void beq(Opcode i); // 0x04
    void bne(Opcode i); // 0x05
    void blez(Opcode i); // 0x06
    void bgtz(Opcode i); // 0x07
    void addiu(Opcode i); // 0x09
    void slti(Opcode i); // 0x0A
    void sltiu(Opcode i); // 0x0B
    void andi(Opcode i); // 0x0C
    void ori(Opcode i); // 0x0D
    void xori(Opcode i); // 0x0E
    void lui(Opcode i); // 0x0F
    void mfc0(Opcode i); // 0x10 0x00
    void mtc0(Opcode i); // 0x10 0x04
	void tlbwi(Opcode i); // 0x10 0x10 0x02
	void eret(Opcode i); // 0x10 0x10 0x18
	void ei(Opcode i); // 0x10 0x10 0x38
	void di(Opcode i); // 0x10 0x10 0x39
    void beql(Opcode i); // 0x14
    void bnel(Opcode i); // 0x15
    void daddiu(Opcode i); // 0x19
    void ldl(Opcode i); // 0x1A
    void ldr(Opcode i); // 0x1B
    void lq(Opcode i); // 0x1E
    void sq(Opcode i); // 0x1F
    void lb(Opcode i); // 0x20
    void lh(Opcode i); // 0x21
    void lw(Opcode i); // 0x23
    void lbu(Opcode i); // 0x24
    void lhu(Opcode i); // 0x25
    void lwu(Opcode i); // 0x27
    void sb(Opcode i); // 0x28
    void sh(Opcode i); // 0x29
    void sw(Opcode i); // 0x2B
    void sdl(Opcode i); // 0x2C
    void sdr(Opcode i); // 0x2D
	void lwc1(Opcode i); // 0x31
    void ld(Opcode i); // 0x37
    void swc1(Opcode i); // 0x39
    void sd(Opcode i); // 0x3f
    
    void sll(Opcode i); // 0x00
    void srl(Opcode i); // 0x02
    void sra(Opcode i); // 0x03
    void sllv(Opcode i); // 0x04
    void srlv(Opcode i); // 0x06
    void srav(Opcode i); // 0x07
    void jr(Opcode i); // 0x08
    void jalr(Opcode i); // 0x09
    void movz(Opcode i); // 0x0a
    void movn(Opcode i); // 0x0b
	void syscall(Opcode i); // 0x0c
    void mfhi(Opcode i); // 0x10
	void mthi(Opcode i); // 0x11
    void mflo(Opcode i); // 0x12
	void mtlo(Opcode i); // 0x13
    void dsllv(Opcode i); // 0x14
    void dsrav(Opcode i); // 0x17
    void mult(Opcode i); // 0x18
    void div(Opcode i); // 0x1A
    void divu(Opcode i); // 0x1B
    void addu(Opcode i); // 0x21
    void subu(Opcode i); // 0x23
    void op_and(Opcode i); // 0x24
    void op_or(Opcode i); // 0x25
    void op_nor(Opcode i); // 0x27
	void mfsa(Opcode i); // 0x28
	void mtsa(Opcode i); // 0x29
    void slt(Opcode i); // 0x2a
    void sltu(Opcode i); // 0x2b
    void daddu(Opcode i); // 0x2d
	void dsubu(Opcode i); // 0x2f
    void dsll(Opcode i); // 0x38
    void dsrl(Opcode i); // 0x3a
    void dsll32(Opcode i); // 0x3c
    void dsrl32(Opcode i); // 0x3e
    void dsra32(Opcode i); // 0x3f

    // RegImm

    void bltz(Opcode i); // 0x00
    void bgez(Opcode i); // 0x01
    void bltzl(Opcode i); // 0x02
    void bgezl(Opcode i); // 0x03
    void bgezal(Opcode i); // 0x11
    void bgezall(Opcode i); // 0x13

    // mmi

	void plzcw(Opcode i); // 0x04
	void mfhi1(Opcode i); // 0x10
	void mthi1(Opcode i); // 0x11
    void mflo1(Opcode i); // 0x12
	void mtlo1(Opcode i); // 0x13
    void mult1(Opcode i); // 0x18
    void divu1(Opcode i); // 0x1b

	// mmi1
	void padduw(Opcode i);

    // mmi3

    void por(Opcode i); // 0x12

    // cop1

    void mfc1(Opcode i);
    void mtc1(Opcode i);
	void ctc1(Opcode i);
	void cfc1(Opcode i);

	// cop1.bc1
	void bc(Opcode i);

    // cop1.w
    void adda(Opcode i);

	// cop2
	void qmfc2(Opcode i);
	void qmtc2(Opcode i);
	void cfc2(Opcode i);
	void ctc2(Opcode i);

    const char* Reg(int index)
    {
        switch (index)
        {
        case 0:
            return "$zero";
        case 1:
            return "$at";
        case 2:
            return "$v0";
        case 3:
            return "$v1";
        case 4:
            return "$a0";
        case 5:
            return "$a1";
        case 6:
            return "$a2";
        case 7:
            return "$a3";
        case 8:
            return "$t0";
        case 9:
            return "$t1";
        case 10:
            return "$t2";
        case 11:
            return "$t3";
        case 12:
            return "$t4";
        case 13:
            return "$t5";
        case 14:
            return "$t6";
        case 15:
            return "$t7";
        case 16:
            return "$s0";
        case 17:
            return "$s1";
        case 18:
            return "$s2";
        case 19:
            return "$s3";
        case 20:
            return "$s4";
        case 21:
            return "$s5";
        case 22:
            return "$s6";
        case 23:
            return "$s7";
        case 24:
            return "$t8";
        case 25:
            return "$t9";
        case 26:
            return "$k0";
        case 27:
            return "$k1";
        case 28:
            return "$gp";
        case 29:
            return "$sp";
        case 30:
            return "$fp";
        case 31:
            return "$ra";
        default:
            return "";
        }
    }

	bool branch_taken = false;

	void exception(int type, bool log);

	void HandleSifSetDma();

	void Write8(uint32_t addr, uint8_t data)
	{
		uint8_t* mem = tlb_map[addr / 4096];
		if (mem > (uint8_t*)1)
		{
			cop0.set_tlb_modified(addr / 4096);
			mem[addr & 4095] = data;
		}
		else if (mem == (uint8_t*)1)
			bus->write<uint8_t>(addr & 0x1FFFFFFF, data);
		else
		{
			printf("Write8 to invalid address 0x%08x\n", addr);
			exit(1);
		}
	}

	void Write16(uint32_t addr, uint16_t data)
	{
		uint8_t* mem = tlb_map[addr / 4096];
		if (mem > (uint8_t*)1)
		{
			cop0.set_tlb_modified(addr / 4096);
			*(uint16_t*)&mem[addr & 4095] = data;
		}
		else if (mem == (uint8_t*)1)
			bus->write<uint16_t>(addr & 0x1FFFFFFF, data);
		else
		{
			printf("Write16 to invalid address 0x%08x\n", addr);
			exit(1);
		}
	}

	void Write32(uint32_t addr, uint32_t data)
	{
		uint8_t* mem = tlb_map[addr / 4096];
		if (mem > (uint8_t*)1)
		{
			cop0.set_tlb_modified(addr / 4096);
			*(uint32_t*)&mem[addr & 4095] = data;
		}
		else if (mem == (uint8_t*)1)
			bus->write<uint32_t>(addr & 0x1FFFFFFF, data);
		else
		{
			printf("Write32 to invalid address 0x%08x\n", addr);
			exit(1);
		}
	}

	void Write64(uint32_t addr, uint64_t data)
	{
		uint8_t* mem = tlb_map[addr / 4096];
		if (mem > (uint8_t*)1)
		{
			cop0.set_tlb_modified(addr / 4096);
			*(uint64_t*)&mem[addr & 4095] = data;
		}
		else if (mem == (uint8_t*)1)
			bus->write<uint64_t>(addr & 0x1FFFFFFF, data);
		else
		{
			printf("Write64 to invalid address 0x%08x\n", addr);
			exit(1);
		}
	}

	void Write128(uint32_t addr, __uint128_t data)
	{
		uint8_t* mem = tlb_map[addr / 4096];
		if (mem > (uint8_t*)1)
		{
			cop0.set_tlb_modified(addr / 4096);
			*(__uint128_t*)&mem[addr & 4095] = data;
		}
		else if (mem == (uint8_t*)1)
			bus->write<__uint128_t>(addr & 0x1FFFFFFF, data);
		else
		{
			printf("Write128 to invalid address 0x%08x\n", addr);
			exit(1);
		}
	}

	uint8_t Read8(uint32_t addr)
	{
		uint8_t* mem = tlb_map[addr / 4096];
		if (mem > (uint8_t*)1)
			return mem[addr & 4095];
		else if (mem == (uint8_t*)1)
			return bus->read<uint8_t>(addr & 0x1FFFFFFF);
		else
		{
			printf("Read8 from invalid address 0x%08x\n", addr);
			exit(1);
		}
	}

	uint16_t Read16(uint32_t addr)
	{
		uint8_t* mem = tlb_map[addr / 4096];
		if (mem > (uint8_t*)1)
			return *(uint16_t*)&mem[addr & 4095];
		else if (mem == (uint8_t*)1)
			return bus->read<uint16_t>(addr & 0x1FFFFFFF);
		else
		{
			printf("Read16 from invalid address 0x%08x\n", addr);
			exit(1);
		}
	}

	uint32_t Read32(uint32_t addr)
	{
		uint8_t* mem = tlb_map[addr / 4096];
		if (mem > (uint8_t*)1)
			return *(uint32_t*)&mem[addr & 4095];
		else if (mem == (uint8_t*)1)
			return bus->read<uint32_t>(addr & 0x1FFFFFFF);
		else
		{
			printf("Read32 from invalid address 0x%08x\n", addr);
			exit(1);
		}
	}

	uint64_t Read64(uint32_t addr)
	{
		uint8_t* mem = tlb_map[addr / 4096];
		if (mem > (uint8_t*)1)
			return *(uint64_t*)&mem[addr & 4095];
		else if (mem == (uint8_t*)1)
			return bus->read<uint64_t>(addr & 0x1FFFFFFF);
		else
		{
			printf("Read64 from invalid address 0x%08x\n", addr);
			exit(1);
		}
	}

	__uint128_t Read128(uint32_t addr)
	{
		uint8_t* mem = tlb_map[addr / 4096];
		if (mem > (uint8_t*)1)
			return *(__uint128_t*)&mem[addr & 4095];
		else if (mem == (uint8_t*)1)
			return bus->read<__uint128_t>(addr & 0x1FFFFFFF);
		else
		{
			printf("Read128 from invalid address 0x%08x\n", addr);
			exit(1);
		}
	}
public:
    EmotionEngine(Bus* bus, VectorUnit* vu0);
	bool int_pending();

	void SetINT1()
	{
		cop0.cop0_regs[13] |= (1 << 11);
	}

	void ClearINT1()
	{
		cop0.cop0_regs[13] &= ~(1 << 11);
	}

    void Clock(int cycles);
    void Dump();
};