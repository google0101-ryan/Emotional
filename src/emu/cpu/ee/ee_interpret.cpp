#include "ee_interpret.h"
#include <util/uint128.h>
#include <string.h>
#include <emu/memory/Bus.h>
#include <emu/cpu/ee/vu.h>

uint128_t regs[32];
uint32_t pc, next_pc;
uint32_t hi, lo;
uint32_t hi1, lo1;
EEInterpreter::COP0 EEInterpreter::cop0;
extern EEInterpreter::Cop1Reg cop1_regs[32];

void EEInterpreter::Reset()
{
    memset(regs, 0, sizeof(regs));

    pc = 0xbfc00000;
    next_pc = pc + 4;

    cop0.prid = 0x2E20;
    
    if (!tlb)
        tlb = new EETLB();
}

void EEInterpreter::Write8(uint32_t addr, uint8_t data)
{
    const auto page = addr >> 12;
    const auto offs = addr & 0xfff;
    const auto ptr = tlb->wrTable[page];

    if (ptr != 0)
        *(uint8_t*)(ptr + offs) = data;
    else
        return Bus::Write8(addr, data); // Slow
}

void EEInterpreter::Write16(uint32_t addr, uint16_t data)
{
    const auto page = addr >> 12;
    const auto offs = addr & 0xfff;
    const auto ptr = tlb->wrTable[page];

    if (ptr != 0)
        *(uint16_t*)(ptr + offs) = data;
    else
        return Bus::Write16(addr, data); // Slow
}

void EEInterpreter::Write32(uint32_t addr, uint32_t data)
{
    const auto page = addr >> 12;
    const auto offs = addr & 0xfff;
    const auto ptr = tlb->wrTable[page];

    if (ptr != 0)
        *(uint32_t*)(ptr + offs) = data;
    else
        return Bus::Write32(addr, data); // Slow
}

void EEInterpreter::Write64(uint32_t addr, uint64_t data)
{
    const auto page = addr >> 12;
    const auto offs = addr & 0xfff;
    const auto ptr = tlb->wrTable[page];

    if (ptr != 0)
        *(uint64_t*)(ptr + offs) = data;
    else
        return Bus::Write64(addr, data); // Slow
}

void EEInterpreter::Write128(uint32_t addr, __uint128_t data)
{
    const auto page = addr >> 12;
    const auto offs = addr & 0xfff;
    const auto ptr = tlb->wrTable[page];

    if (ptr != 0)
        *(__uint128_t*)(ptr + offs) = data;
    else
        return Bus::Write128(addr, {data}); // Slow
}

uint8_t EEInterpreter::Read8(uint32_t addr)
{
    const auto page = addr >> 12;
    const auto offs = addr & 0xfff;
    const auto ptr = tlb->rdTable[page];

    if (ptr != 0)
        return *(uint8_t*)(ptr + offs);
    else
        return Bus::Read8(addr); // Slow
}

uint16_t EEInterpreter::Read16(uint32_t addr)
{
    const auto page = addr >> 12;
    const auto offs = addr & 0xfff;
    const auto ptr = tlb->rdTable[page];

    if (ptr != 0)
        return *(uint16_t*)(ptr + offs);
    else
        return Bus::Read16(addr); // Slow
}

uint32_t EEInterpreter::Read32(uint32_t addr)
{
    const auto page = addr >> 12;
    const auto offs = addr & 0xfff;
    const auto ptr = tlb->rdTable[page];

    if (ptr != 0)
        return *(uint32_t*)(ptr + offs);
    else
        return Bus::Read32(addr); // Slow
}

uint64_t EEInterpreter::Read64(uint32_t addr)
{
    const auto page = addr >> 12;
    const auto offs = addr & 0xfff;
    const auto ptr = tlb->rdTable[page];

    if (ptr != 0)
        return *(uint64_t*)(ptr + offs);
    else
        return Bus::Read64(addr); // Slow
}

__uint128_t EEInterpreter::Read128(uint32_t addr)
{
    const auto page = addr >> 12;
    const auto offs = addr & 0xfff;
    const auto ptr = tlb->rdTable[page];

    if (ptr != 0)
        return *(__uint128_t*)(ptr + offs);
    else
        return Bus::Read128(addr).u128; // Slow
}

void EETLB::Unmap(TlbEntry& entry)
{
    uint32_t real_vpn = entry.entry.vpn2 * 2;
    real_vpn >>= entry.pageShift;

    uint32_t even_page = (real_vpn * entry.pageSize) / 4096;
    uint32_t odd_page = ((real_vpn + 1) * entry.pageSize) / 4096;

    if (entry.entry.s)
    {
        if (entry.entry.v0)
        {
            for (uint32_t i = 0; i < 1024*16; i += 4096)
            {
                int index = i / 4096;
                rdTable[even_page + index] = 0;
                wrTable[even_page + index] = 0;
            }
        }
    }
    else
    {
        if (entry.entry.v0)
        {
            for (uint32_t i = 0; i < entry.pageSize; i += 4096)
            {
                int index = i / 4096;
                rdTable[even_page + index] = 0;
                wrTable[even_page + index] = 0;
                rdTable[odd_page + index] = 0;
                wrTable[odd_page + index] = 0;
            }
        }
        if (entry.entry.v1)
        {
            for (uint32_t i = 0; i < entry.pageSize; i += 4096)
            {
                int index = i / 4096;
                rdTable[odd_page + index] = 0;
                wrTable[odd_page + index] = 0;
            }
        }
    }
}

void EETLB::DoRemap(int index)
{
    auto& tlbEntry = tlb[index];

    uint32_t entryLo0 = EEInterpreter::cop0.regs[2];
    uint32_t entryLo1 = EEInterpreter::cop0.regs[3];
    uint32_t pageMask = EEInterpreter::cop0.regs[5];
    uint32_t entryHi = EEInterpreter::cop0.regs[10];

    Unmap(tlbEntry);

    switch (pageMask >> 13)
    {
    case 0:
        tlbEntry.pageSize = 4*1024;
        tlbEntry.pageShift = 0;
        break;
    case 3:
        tlbEntry.pageSize = 16*1024;
        tlbEntry.pageShift = 2;
        break;
    case 0xf:
        tlbEntry.pageSize = 64*1024;
        tlbEntry.pageShift = 4;
        break;
    case 0x3f:
        tlbEntry.pageSize = 256*1024;
        tlbEntry.pageShift = 6;
        break;
    case 0xff:
        tlbEntry.pageSize = 1024*1024;
        tlbEntry.pageShift = 8;
        break;
    case 0x3ff:
        tlbEntry.pageSize = 4*1024*1024;
        tlbEntry.pageShift = 10;
        break;
    case 0xfff:
        tlbEntry.pageSize = 16*1024*1024;
        tlbEntry.pageShift = 12;
        break;
    default:
        printf("Unknown page size 0x%08x\n", pageMask >> 13);
        exit(1);
    }

    uint32_t pfn0 = (entryLo0 >> 6) & 0xFFFFF;
    uint32_t pfn1 = (entryLo1 >> 6) & 0xFFFFF;
    uint32_t vpn2 = (entryHi >> 13) & 0x7FFFF;

    tlbEntry.entry.g = (entryLo0 & 1) & (entryLo1 & 1);
    tlbEntry.entry.v0 = (entryLo0 >> 1) & 1;
    tlbEntry.entry.d0 = (entryLo0 >> 2) & 1;
    tlbEntry.entry.c0 = (entryLo0 >> 3) & 0x7;
    tlbEntry.entry.pfn0 = (entryLo0 >> 6) & 0xFFFFF;
    tlbEntry.entry.v1 = (entryLo1 >> 1) & 1;
    tlbEntry.entry.d1 = (entryLo1 >> 2) & 1;
    tlbEntry.entry.c1 = (entryLo1 >> 3) & 0x7;
    tlbEntry.entry.pfn1 = (entryLo1 >> 6) & 0xFFFFF;

    tlbEntry.entry.vpn2 = entryHi >> 13;
    tlbEntry.entry.asid = entryHi & 0xff;

    uint32_t real_virt = vpn2 * 2;
    real_virt >>= tlbEntry.pageShift;
    real_virt *= tlbEntry.pageSize;

    uint32_t real_phy = pfn0 >> tlbEntry.pageShift;
    real_phy *= tlbEntry.pageSize;
    uint32_t real_phy_1 = pfn1 >> tlbEntry.pageShift;
    real_phy_1 *= tlbEntry.pageSize;
    
    uint32_t real_vpn = (vpn2 * 2) >> tlbEntry.pageShift;

    uint32_t even_virt_page = (real_vpn * tlbEntry.pageSize) / 4096;
    uint32_t even_virt_addr = even_virt_page * 4096;
    uint32_t even_phy_addr = (pfn0 >> tlbEntry.pageShift) * tlbEntry.pageSize;
    uint32_t odd_virt_page = ((real_vpn+1) * tlbEntry.pageSize) / 4096;
    uint32_t odd_virt_addr = odd_virt_page * 4096;
    uint32_t odd_phy_addr = (pfn1 >> tlbEntry.pageShift) * tlbEntry.pageSize;

    if (tlbEntry.entry.s)
    {
        if (tlbEntry.entry.v0)
        {
            for (uint32_t i = 0; i < 1024*16; i += 4096)
            {
                int index = i / 4096;
                rdTable[even_virt_page + index] = (uintptr_t)(Bus::GetSprPtr()+i);
                wrTable[even_virt_page + index] = (uintptr_t)(Bus::GetSprPtr()+i);
            }
        }
    }
    else
    {
        if (tlbEntry.entry.v0)
        {
            for (uint32_t i = 0; i < tlbEntry.pageSize; i += 4096)
            {
                int index = i / 4096;
                uintptr_t mem = (uintptr_t)Bus::GetPtrForAddress(even_phy_addr+i);
                rdTable[even_virt_page + index] = mem;
                wrTable[even_virt_page + index] = mem;
            }
        }
        if (tlbEntry.entry.v1)
        {
            for (uint32_t i = 0; i < tlbEntry.pageSize; i += 4096)
            {
                int index = i / 4096;
                uintptr_t mem = (uintptr_t)Bus::GetPtrForAddress(odd_phy_addr+i);
                rdTable[odd_virt_page + index] = mem;
                wrTable[odd_virt_page + index] = mem;
            }
        }
    }
}

void EEInterpreter::Clock(int cycles)
{
    for (int cycle = 0; cycle < cycles; cycle++)
    {
        uint32_t instr = Read32(pc);

        pc = next_pc;
        next_pc += 4;

        if (instr == 0)
        {
            if constexpr (CanDisassamble)
                printf("[emu/EE]: nop\n");
            continue;
        }

        uint32_t opcode = (instr >> 26) & 0x3F;
        switch (opcode)
        {
        case 0x00:
        {
            opcode = instr & 0x3F;
            switch (opcode)
            {
            case 0x00:
                Sll(instr);
                break;
            case 0x02:
                Srl(instr);
                break;
            case 0x03:
                Sra(instr);
                break;
            case 0x04:
                Sllv(instr);
                break;
            case 0x07:
                Srav(instr);
                break;
            case 0x08:
                Jr(instr);
                break;
            case 0x09:
                Jalr(instr);
                break;
            case 0x0a:
                Movz(instr);
                break;
            case 0x0b:
                Movn(instr);
                break;
            case 0x0f:
                if constexpr (CanDisassamble)
                    printf("[emu/EE]: sync.p\n");
                break;
            case 0x10:
                Mfhi(instr);
                break;
            case 0x12:
                Mflo(instr);
                break;
            case 0x14:
                Dsllv(instr);
                break;
            case 0x17:
                Dsrav(instr);
                break;
            case 0x18:
                Mult(instr);
                break;
            case 0x19:
                Multu(instr);
                break;
            case 0x1a:
                Div(instr);
                break;
            case 0x1b:
                Divu(instr);
                break;
            case 0x21:
                Addu(instr);
                break;
            case 0x23:
                Subu(instr);
                break;
            case 0x24:
                And(instr);
                break;
            case 0x25:
                Or(instr);
                break;
            case 0x27:
                Nor(instr);
                break;
            case 0x2a:
                Slt(instr);
                break;
            case 0x2b:
                Sltu(instr);
                break;
            case 0x2d:
                Daddu(instr);
                break;
            case 0x38:
                Dsll(instr);
                break;
            case 0x3c:
                Dsll32(instr);
                break;
            case 0x3f:
                Dsra32(instr);
                break;
            default:
                printf("[emu/EE]: Unknown special instruction 0x%02x\n", opcode);
                exit(1);
            }
            break;
        }
        case 0x01:
        {
            opcode = (instr >> 16) & 0x1F;
            switch (opcode)
            {
            case 0x00:
                Bltz(instr);
                break;
            case 0x01:
                Bgez(instr);
                break;
            default:
                printf("[emu/EE]: Unknown regimm instruction 0x%02x\n", opcode);
                exit(1);
            }
            break;
        }
        case 0x02:
            J(instr);
            break;
        case 0x03:
            Jal(instr);
            break;
        case 0x04:
            Beq(instr);
            break;
        case 0x05:
            Bne(instr);
            break;
        case 0x06:
            Blez(instr);
            break;
        case 0x07:
            Bgtz(instr);
            break;
        case 0x09:
            Addiu(instr);
            break;
        case 0x0a:
            Slti(instr);
            break;
        case 0x0b:
            Sltiu(instr);
            break;
        case 0x0c:
            Andi(instr);
            break;
        case 0x0d:
            Ori(instr);
            break;
        case 0x0e:
            Xori(instr);
            break;
        case 0x0f:
            Lui(instr);
            break;
        case 0x10:
        {
            opcode = (instr >> 21) & 0x3F;
            switch (opcode)
            {
            case 0x00:
                Mfc0(instr);
                break;
            case 0x04:
                Mtc0(instr);
                break;
            case 0x10:
            {
                opcode = instr & 0x3F;
                switch (opcode)
                {
                case 0x02:
                    Tlbwi(instr);
                    break;
                default:
                    printf("[emu/EE]: Unknown cop0 tlb instruction 0x%02x\n", opcode);
                    exit(1);
                }
                break;
            }
            default:
                printf("[emu/EE]: Unknown cop0 instruction 0x%02x\n", opcode);
                exit(1);
            }
            break;
        }
        case 0x11:
        {
            opcode = (instr >> 21) & 0x1F;
            switch (opcode)
            {
            case 0x04:
                Mtc1(instr);
                break;
            case 0x06:
                break;
            case 0x10:
            {
                opcode = instr & 0x3F;
                switch (opcode)
                {
                case 0x18:
                    Addas(instr);
                    break;
                default:
                    printf("[emu/EE]: Unknown cop1.s instruction 0x%02x\n", opcode);
                    exit(1);
                }
                break;
            }
            default:
                printf("[emu/EE]: Unknown cop1 instruction 0x%02x\n", opcode);
                exit(1);
            }
            break;
        }
        case 0x12:
        {
            opcode = (instr >> 21) & 0x3F;
            switch (opcode)
            {
            case 0x01:
                Qmfc2(instr);
                break;
            case 0x02:
                Cfc2(instr);
                break;
            case 0x05:
                Qmtc2(instr);
                break;
            case 0x06:
                Ctc2(instr);
                break;
            case 0x10 ... 0x1F:
            {
                opcode = instr & 0x3F;
                switch (opcode)
                {
                case 0x2c:
                    VectorUnit::VU0::Vsub(instr);
                    break;
                case 0x30:
                    VectorUnit::VU0::Viadd(instr);
                    break;
                case 0x3C ... 0x3F:
                {
                    opcode = (instr & 0x3) | ((instr >> 6) & 0x1F) << 2;
                    switch (opcode)
                    {
                    case 0x35:
                        VectorUnit::VU0::Vsqi(instr);
                        break;
                    case 0x3f:
                        VectorUnit::VU0::Viswr(instr);
                        break;
                    default:
                        printf("[emu/EE]: Unknown cop2 special2 instruction 0x%02x\n", opcode);
                        exit(1);
                    }
                    break;
                }
                default:
                    printf("[emu/EE]: Unknown cop2 special1 instruction 0x%02x\n", opcode);
                    exit(1);
                }
                break;
            }
            default:
                printf("[emu/EE]: Unknown cop2 instruction 0x%02x\n", opcode);
                exit(1);
            }
            break;
        }
        case 0x14:
            Beql(instr);
            break;
        case 0x15:
            Bnel(instr);
            break;
        case 0x19:
            Daddiu(instr);
            break;
        case 0x1C:
        {
            opcode = instr & 0x3F;
            switch (opcode)
            {
            case 0x12:
                Mflo1(instr);
                break;
            case 0x18:
                Mult1(instr);
                break;
            case 0x1b:
                Divu1(instr);
                break;
            case 0x29:
            {
                opcode = (instr >> 6) & 0x1F;
                switch (opcode)
                {
                case 0x12:
                    Por(instr);
                    break;
                default:
                    printf("[emu/EE]: Unknown mmi3 instruction 0x%02x\n", opcode);
                    exit(1);
                }
                break;
            }
            default:
                printf("[emu/EE]: Unknown mmi instruction 0x%02x\n", opcode);
                exit(1);
            }
            break;
        }
        case 0x1e:
            Lq(instr);
            break;
        case 0x1f:
            Sq(instr);
            break;
        case 0x20:
            Lb(instr);
            break;
        case 0x21:
            Lh(instr);
            break;
        case 0x23:
            Lw(instr);
            break;
        case 0x24:
            Lbu(instr);
            break;
        case 0x25:
            Lhu(instr);
            break;
        case 0x27:
            Lwu(instr);
            break;
        case 0x28:
            Sb(instr);
            break;
        case 0x29:
            Sh(instr);
            break;
        case 0x2b:
            Sw(instr);
            break;
        case 0x2f:
            if constexpr (CanDisassamble)
                printf("cache\n");
            break;
        case 0x37:
            Ld(instr);
            break;
        case 0x39:
            Swc1(instr);
            break;
        case 0x3f:
            Sd(instr);
            break;
        default:
            printf("[emu/EE]: Unknown instruction 0x%02x\n", opcode);
            exit(1);
        }

        cop0.count++;
    }
}

extern float convert(uint32_t);

void EEInterpreter::Dump()
{
    for (int i = 0; i < 32; i++)
        printf("[emu/EE]: %s\t->\t%s\n", Reg(i), print_128(regs[i]));
    for (int i = 0; i < 32; i++)
        printf("[emu/EE]: f%d\t->\t%f\n", i, convert(cop1_regs[i].u));
    printf("pc\t->\t0x%08x\n", pc);
    printf("next pc\t->\t0x%08x\n", next_pc);
    printf("hi\t->\t0x%08x\n", hi);
    printf("lo\t->\t0x%08x\n", lo);
    printf("hi1\t->\t0x%08x\n", hi1);
    printf("lo1\t->\t0x%08x\n", lo1);
}

const uint32_t PAGE_SIZE = 4 * 1024;

EETLB::EETLB()
{
    wrTable = new uintptr_t[0x100000];
    rdTable = new uintptr_t[0x100000];

    memset(wrTable, 0, sizeof(uintptr_t)*0x100000);
    memset(rdTable, 0, sizeof(uintptr_t)*0x100000);

    for (auto pageIndex = 0; pageIndex < 8192; pageIndex++)
    {
        const auto ptr = (uintptr_t)(&(Bus::GetRamPtr()[(pageIndex * PAGE_SIZE) & 0x1FFFFFF]));
        rdTable[pageIndex + 0x00000] = ptr;
        rdTable[pageIndex + 0x80000] = ptr;
        rdTable[pageIndex + 0xA0000] = ptr;
        wrTable[pageIndex + 0x00000] = ptr;
        wrTable[pageIndex + 0x80000] = ptr;
        wrTable[pageIndex + 0xA0000] = ptr;
    }

    for (auto pageIndex = 0; pageIndex < 1024; pageIndex++)
    {
        const auto ptr = (uintptr_t)(&BiosRom[(pageIndex * PAGE_SIZE) & 0x1FFFFFF]);
        rdTable[pageIndex + 0x1FC00] = ptr;
        rdTable[pageIndex + 0x9FC00] = ptr;
        rdTable[pageIndex + 0xBFC00] = ptr;
    }

    memset(tlb, 0, sizeof(tlb));
}
