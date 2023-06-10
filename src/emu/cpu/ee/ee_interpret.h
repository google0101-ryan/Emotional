#pragma once

#include <stdint.h>

class EETLB
{
public:
    uintptr_t* rdTable;
    uintptr_t* wrTable;

    struct TlbEntry
    {
        union Entry
        {
            __uint128_t data;
            struct
            {
                __uint128_t v0 : 1,
                d0 : 1,
                c0 : 3,
                pfn0 : 20,
                v1 : 1,
                d1 : 1,
                c1 : 3,
                pfn1 : 20,
                s : 1,
                asid : 8,
                g : 1,
                vpn2 : 19,
                mask : 12;
            };
        } entry;
        uint32_t pageSize;
        uint32_t pageShift;
    } tlb[48];
public:
    EETLB();

    void Unmap(TlbEntry& entry);
    void DoRemap(int index);
};

namespace EEInterpreter
{

inline const char* Reg(int index)
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

    enum OperatingMode 
    {
        USER_MODE = 0b10,
        SUPERVISOR_MODE = 0b01,
        KERNEL_MODE = 0b00
    };

    /* The COP0 registers */
    union COP0
    {
        uint32_t regs[32] = {};
        struct
        {
            uint32_t index;
            uint32_t random;
            uint32_t entry_lo0;
            uint32_t entry_lo1;
            uint32_t context;
            uint32_t page_mask;
            uint32_t wired;
            uint32_t reserved0[1];
            uint32_t bad_vaddr;
            uint32_t count;
            uint32_t entryhi;
            uint32_t compare;
            COP0Status status;
            COP0Cause cause;
            uint32_t epc;
            uint32_t prid;
            uint32_t config;
            uint32_t reserved1[6];
            uint32_t bad_paddr;
            uint32_t debug;
            uint32_t perf;
            uint32_t reserved2[2];
            uint32_t tag_lo;
            uint32_t tag_hi;
            uint32_t error_epc;
            uint32_t reserved3[1];
        };
    };

    extern COP0 cop0;
    static constexpr bool CanDisassamble = false;

    extern EETLB* tlb;


union Cop1Reg
{
    float f;
    uint32_t u;
    int32_t s;
};

uint8_t Read8(uint32_t addr);
uint16_t Read16(uint32_t addr);
uint32_t Read32(uint32_t addr);
uint64_t Read64(uint32_t addr);
__uint128_t Read128(uint32_t addr);

void Write8(uint32_t addr, uint8_t data);
void Write16(uint32_t addr, uint16_t data);
void Write32(uint32_t addr, uint32_t data);
void Write64(uint32_t addr, uint64_t data);
void Write128(uint32_t addr, __uint128_t data);

void Reset();

void Clock(int cycles);
void Dump();

// Normal
void J(uint32_t instr); // 0x02
void Jal(uint32_t instr); // 0x03
void Beq(uint32_t instr); // 0x04
void Bne(uint32_t instr); // 0x05
void Blez(uint32_t instr); // 0x06
void Bgtz(uint32_t instr); // 0x07
void Addiu(uint32_t instr); // 0x09
void Slti(uint32_t instr); // 0x0A
void Sltiu(uint32_t instr); // 0x0B
void Andi(uint32_t instr); // 0x0C
void Ori(uint32_t instr); // 0x0D
void Xori(uint32_t instr); // 0x0E
void Lui(uint32_t instr); // 0x0F
void Beql(uint32_t instr); // 0x14
void Bnel(uint32_t instr); // 0x15
void Daddiu(uint32_t instr); // 0x19
void Lq(uint32_t instr); // 0x1e
void Sq(uint32_t instr); // 0x1f
void Lb(uint32_t instr); // 0x20
void Lh(uint32_t instr); // 0x21
void Lw(uint32_t instr); // 0x23
void Lbu(uint32_t instr); // 0x24
void Lhu(uint32_t instr); // 0x25
void Lwu(uint32_t instr); // 0x27
void Sb(uint32_t instr); // 0x28
void Sh(uint32_t instr); // 0x29
void Sw(uint32_t instr); // 0x2B
void Swc1(uint32_t instr); // 0x31
void Ld(uint32_t instr); // 0x37
void Sd(uint32_t instr); // 0x3F

// Special
void Sll(uint32_t instr); // 0x00
void Srl(uint32_t instr); // 0x02
void Sra(uint32_t instr); // 0x03
void Sllv(uint32_t instr); // 0x04
void Srav(uint32_t instr); // 0x07
void Jr(uint32_t instr); // 0x08
void Jalr(uint32_t instr); // 0x09
void Movz(uint32_t instr); // 0x0a
void Movn(uint32_t instr); // 0x0b
void Mfhi(uint32_t instr); // 0x10
void Mflo(uint32_t instr); // 0x12
void Dsllv(uint32_t instr); // 0x14
void Dsrav(uint32_t instr); // 0x17
void Mult(uint32_t instr); // 0x18
void Multu(uint32_t instr); // 0x19
void Div(uint32_t instr); // 0x1A
void Divu(uint32_t instr); // 0x1B
void Addu(uint32_t instr); // 0x21
void Subu(uint32_t instr); // 0x23
void And(uint32_t instr); // 0x24
void Or(uint32_t instr); // 0x25
void Nor(uint32_t instr); // 0x27
void Slt(uint32_t instr); // 0x2a
void Sltu(uint32_t instr); // 0x2b
void Daddu(uint32_t instr); // 0x2d
void Dsll(uint32_t instr); // 0x38
void Dsll32(uint32_t instr); // 0x3c
void Dsra32(uint32_t instr); // 0x3f

// Regimm
void Bltz(uint32_t instr); // 0x00
void Bgez(uint32_t instr); // 0x01

// Cop0
void Mfc0(uint32_t instr); // 0x00
void Mtc0(uint32_t instr); // 0x04

// Cop0 TLB
void Tlbwi(uint32_t instr); // 0x02

// MMI
void Mflo1(uint32_t instr); // 0x12
void Mult1(uint32_t instr); // 0x18
void Divu1(uint32_t instr); // 0x1B

// MMI3
void Por(uint32_t instr);

// COP2
void Qmfc2(uint32_t instr); // 0x01
void Cfc2(uint32_t instr); // 0x02
void Qmtc2(uint32_t instr); // 0x05
void Ctc2(uint32_t instr); // 0x06

// COP1
void Mtc1(uint32_t instr); // 0x04

// COP1.S
void Addas(uint32_t instr); // 0x18

}