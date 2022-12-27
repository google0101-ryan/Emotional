#pragma once

#include <cstdint>
#include <emu/cpu/opcode.h>

class Bus;

class IoProcessor
{
private:
    uint32_t pc;
    uint32_t regs[32];
    uint32_t hi, lo;

    union COP0STAT 
	{
		uint32_t value;
		struct 
		{
			uint32_t IEc : 1;		/* Interrupt Enable (current) */
			uint32_t KUc : 1;		/* Kernel-User Mode (current) */
			uint32_t IEp : 1;		/* Interrupt Enable (previous) */
			uint32_t KUp : 1;		/* Kernel-User Mode (previous) */
			uint32_t IEo : 1;		/* Interrupt Enable (old) */
			uint32_t KUo : 1;		/* Kernel-User Mode (old) */
			uint32_t : 2;
			uint32_t Im : 8;	/* Hardware Interrupt Mask */
			uint32_t IsC : 1;		/* Isolate Cache */
			uint32_t : 1;
			uint32_t PZ : 1;		/* Parity Zero */
			uint32_t CM : 1;
			uint32_t PE : 1;		/* Parity Error */
			uint32_t TS : 1;		/* TLB Shutdown */
			uint32_t BEV : 1;		/* Bootstrap Exception Vectors */
			uint32_t : 5;
			uint32_t Cu : 4;		/* Coprocessor Usability */
		};
	};

	union COP0CAUSE 
	{
		uint32_t value;
		struct 
		{
			uint32_t : 2;
			uint32_t excode : 5;	/* Exception Code */
			uint32_t : 1;
			uint32_t IP : 8;		/* Interrupt Pending */
			uint32_t : 12;
			uint32_t CE : 2;		/* Coprocessor Error */
			uint32_t BT : 1;		/* Branch Taken */
			uint32_t BD : 1;		/* Branch Delay */
		};
	};

    union COP0
    {
        uint32_t regs[32] = {};
		struct 
		{
			uint32_t r0;
			uint32_t r1;
			uint32_t r2;
			uint32_t Bpc;		/* Breakpoint Program Counter */
			uint32_t r4;
			uint32_t BDA;		/* Breakpoint Data Address */
			uint32_t TAR;		/* Target Address */
			uint32_t DCIC;		/* Debug and Cache Invalidate Control */
			uint32_t BadA;		/* Bad Address */
			uint32_t BDAM;		/* Breakpoint Data Address Mask */
			uint32_t r10;
			uint32_t BpcM;		/* Breakpoint Program Counter Mask */
			COP0STAT status;	/* Status */
			COP0CAUSE cause;	/* Cause */
			uint32_t epc;		/* Exception Program Counter */
			uint32_t PRId;		/* Processor Revision Identifier */
		};
    } Cop0;

    Bus* bus;

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

    struct LoadDelay
    {
        int reg = 0;
        uint32_t data = 0;
	} write_back, memory_load, delayed_memory_load;

    bool singleStep = false;

    void set_reg(uint32_t regN, uint32_t value)
    {
        write_back.reg = regN;
        write_back.data = value;
    }

	void op_special(); // 0x00
    void op_bcond(); // 0x01
    void op_j(); // 0x02
    void op_jal(); // 0x03
    void op_beq(); // 0x04
    void op_bne(); // 0x05
    void op_blez(); // 0x06
    void op_bgtz(); // 0x07
    void op_addi(); // 0x08
    void op_addiu(); // 0x09
    void op_slti(); // 0x0A
    void op_sltiu(); // 0x0B
    void op_andi(); // 0x0C
    void op_ori(); // 0x0D
    void op_lui(); // 0x0F
    void op_cop0(); // 0x10
    void op_lb(); // 0x20
    void op_lh(); // 0x21
    void op_lw(); // 0x23
    void op_lbu(); // 0x24
    void op_lhu(); // 0x25
    void op_sb(); // 0x28
    void op_sh(); // 0x29
    void op_sw(); // 0x2B

    // special

    void op_sll(); // 0x00
    void op_srl(); // 0x02
    void op_sra(); // 0x03
    void op_sllv(); // 0x04
    void op_srlv(); // 0x06
    void op_jr(); // 0x08
    void op_jalr(); // 0x09
    void op_syscall(); // 0x0C
    void op_mfhi(); // 0x10
    void op_mthi(); // 0x11
    void op_mflo(); // 0x12
    void op_mtlo(); // 0x13
    void op_mult(); // 0x18
    void op_multu(); // 0x19
    void op_divu(); // 0x1B
    void op_add(); // 0x20
    void op_addu(); // 0x21
    void op_subu(); // 0x23
    void op_and(); // 0x24
    void op_or(); // 0x25
    void op_xor(); // 0x26
    void op_nor(); // 0x27
    void op_slt(); // 0x2A
    void op_sltu(); // 0x2B

    // cop0

    void op_mfc0(); // 0x00
    void op_mtc0(); // 0x04

	void direct_jump();
	void handle_load_delay();
	void branch();
	void load(uint32_t regN, uint32_t value);

    bool isCacheIsolated() {return Cop0.status.IsC;}

	Opcode i, next_instr;
public:
    IoProcessor(Bus* bus);

    void Clock(int cycles);
    void Dump();

    bool IntPending();
};