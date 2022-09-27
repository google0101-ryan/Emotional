#pragma once

#include <cstdint>
#include <emu/cpu/opcode.h>

class Bus;

class IoProcessor
{
private:
    uint32_t pc, next_pc;
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
        int reg;
        uint32_t data;
    } load_delay, next_load_delay, write_back;

    bool singleStep = false;

    void set_reg(uint32_t regN, uint32_t value)
    {
        write_back.reg = regN;
        write_back.data = value;
    }

    void bcond(Opcode i); // 0x01
    void j(Opcode i); // 0x02
    void jal(Opcode i); // 0x03
    void beq(Opcode i); // 0x04
    void bne(Opcode i); // 0x05
    void blez(Opcode i); // 0x06
    void bgtz(Opcode i); // 0x07
    void addi(Opcode i); // 0x08
    void addiu(Opcode i); // 0x09
    void slti(Opcode i); // 0x0A
    void sltiu(Opcode i); // 0x0B
    void andi(Opcode i); // 0x0C
    void ori(Opcode i); // 0x0D
    void lui(Opcode i); // 0x0F
    void cop0(Opcode i); // 0x10
    void lb(Opcode i); // 0x20
    void lh(Opcode i); // 0x21
    void lw(Opcode i); // 0x23
    void lbu(Opcode i); // 0x24
    void lhu(Opcode i); // 0x25
    void sb(Opcode i); // 0x28
    void sh(Opcode i); // 0x29
    void sw(Opcode i); // 0x2B

    // special

    void sll(Opcode i); // 0x00
    void srl(Opcode i); // 0x02
    void sra(Opcode i); // 0x03
    void sllv(Opcode i); // 0x04
    void srlv(Opcode i); // 0x06
    void jr(Opcode i); // 0x08
    void jalr(Opcode i); // 0x09
    void syscall(Opcode i); // 0x0C
    void mfhi(Opcode i); // 0x10
    void mthi(Opcode i); // 0x11
    void mflo(Opcode i); // 0x12
    void mtlo(Opcode i); // 0x13
    void mult(Opcode i); // 0x18
    void multu(Opcode i); // 0x19
    void divu(Opcode i); // 0x1B
    void add(Opcode i); // 0x20
    void addu(Opcode i); // 0x21
    void subu(Opcode i); // 0x23
    void op_and(Opcode i); // 0x24
    void op_or(Opcode i); // 0x25
    void op_xor(Opcode i); // 0x26
    void op_nor(Opcode i); // 0x27
    void slt(Opcode i); // 0x2A
    void sltu(Opcode i); // 0x2B

    // cop0

    void mfc0(Opcode i); // 0x00
    void mtc0(Opcode i); // 0x04

    void AdvancePC();

    bool isCacheIsolated() {return Cop0.regs[12] & (1 << 16);}
public:
    IoProcessor(Bus* bus);

    void Clock(int cycles);
    void Dump();

    bool IntPending();
};