#pragma once

#include <emu/iopbus.h>
#include <bits/stdint-uintn.h>
#include <string>
#include <fstream>
#include <emu/gs/gif.h>
#include <emu/gs/gs.h>
#include <emu/cpu/ee/timers.h>
#include <emu/cpu/ee/dmac.h>
#include <emu/cpu/ee/vu.h>
#include <emu/cpu/ee/vif.h>
#include <emu/cpu/iop/iop.h>
#include <emu/sif.h>

class Bus
{
private:
    uint8_t ram[0x2000000];
    uint8_t bios[0x400000];
    uint8_t scratchpad[0x4000];
    uint8_t iop_ram[0x200000];

    uint32_t MCH_DRD, MCH_RICM, rdram_sdevid;
    uint32_t intc_stat, intc_mask;

    std::ofstream console;

    GIF* gif;
    GraphicsSynthesizer* gs;
    EmotionTimers* ee_timers;
    EmotionDma* ee_dmac;
    VectorUnit* vu0, *vu1;
    VectorInterface* vif0, *vif1;
    SubsystemInterface* sif;
    IopBus* iop_bus;
    IoProcessor* iop;

    uint32_t Translate(uint32_t addr)
    {
        constexpr uint32_t KUSEG_MASKS[8] = 
        {
            /* KUSEG: Don't touch the address, it's fine */
            0xffffffff, 0xfffffff, 0xfffffff, 0xffffffff,
            /* KSEG0: Strip the MSB (0x8 -> 0x0 and 0x9 -> 0x1) */
            0x7fffffff,
            /* KSEG1: Strip the 3 MSB's (0xA -> 0x0 and 0xB -> 0x1) */
            0x1fffffff,
            /* KSEG2: Don't touch the address, it's fine */
            0xffffffff, 0x1fffffff,
        };

        addr &= KUSEG_MASKS[addr >> 29];

		return addr;
    }
public:
    Bus(std::string biosName, bool& success);

    void Clock();

    void IopPrint(uint32_t pointer, uint32_t text_size)
    {
        while (text_size)
        {
            auto c = (char)iop_ram[pointer & 0x1FFFFF];
            iop_bus->OutputToLog(c);
            pointer++;
            text_size--;
        }
    }

	uint8_t* grab_ee_ram() {return ram;}

    template<typename T>
    T read_iop(uint32_t addr)
    {
        return iop_bus->read<T>(addr);
    }

    template<typename T>
    void write_iop(uint32_t addr, T data)
    {
        return iop_bus->write(addr, data);
    }

    template<typename T>
    T read(uint32_t addr)
    {
        addr = Translate(addr);
        
        if (addr >= 0x1FC00000 && addr < 0x21C00000)
            return *(T*)&bios[addr - 0x1FC00000];
        if (addr >= 0x70000000 && addr < 0x70004000)
            return *(T*)&scratchpad[addr - 0x70000000];   
        if (addr < 0x2000000)
            return *(T*)&ram[addr];
        if (addr >= 0x1c000000 && addr < 0x1e000000)
            return *(T*)&iop_ram[addr - 0x1c000000];
        else if (addr >= 0x10003000 && addr <= 0x10006000)
            return (T)gif->read32(addr);
        else if (addr >= 0x1000E000 && addr <= 0x1000E060)
            return (T)ee_dmac->read_dma(addr);
        else if (addr >= 0x10002000 && addr <= 0x10002030)
            return 0;
        else if (addr >= 0x10008000 && addr <= 0x1000D480)
            return (T)ee_dmac->read(addr);
        else if (addr >= 0x1000F200 && addr <= 0x1000F260)
            return (T)sif->read(addr);

        switch (addr)
        {
        case 0x1000f000:
            return intc_stat;
        case 0x1000f010:
            return intc_mask;
        case 0x1000f130: // SIO status
        case 0x1000f400:
        case 0x1000f410:
        case 0x1000f430:
            return 0;
        case 0x1000f440: /* Read from MCH_RICM */
        {
            uint8_t SOP = (MCH_RICM >> 6) & 0xF;
            uint8_t SA = (MCH_RICM >> 16) & 0xFFF;
            if (!SOP)
            {
                switch (SA)
                {
                case 0x21:
                    if (rdram_sdevid < 2)
                    {
                        rdram_sdevid++;
                        return 0x1F;
                    }
                    return 0;
                case 0x23:
                    return (T)0x0D0D;
                case 0x24:
                    return 0x0090;
                case 0x40:
                    return MCH_RICM & 0x1F;
                }
            }
            return 0;
        }
        case 0x1f80141c:
        case 0x1f803800:
            return 0;
        }
        
        printf("[emu/Bus]: %s: Failed to read from address 0x%08x\n", __FUNCTION__, addr);
        exit(1);
    }

    template<typename T>
    void write(uint32_t addr, T data)
    {
        addr = Translate(addr);

        if (addr >= 0x70000000 && addr < 0x70004000)
        {
            *(T*)&scratchpad[addr - 0x70000000] = data;
            return;
        }
        else if (addr < 0x2000000)
        {
            *(T*)&ram[addr] = data;
            return;
        }
        else if (addr >= 0x12000000 && addr <= 0x12002000)
        {
            gs->write32(addr, data);
            return;
        }
        else if (addr >= 0x10000000 && addr <= 0x10001830)
        {
            ee_timers->write(addr, data);
            return;
        }
        else if (addr >= 0x10008000 && addr <= 0x1000D480)
        {
            ee_dmac->write(addr, data);
            return;
        }
        else if (addr >= 0x1000E000 && addr <= 0x1000E050)
        {
            ee_dmac->write_dma(addr, data);
            return;
        }
        else if (addr >= 0x11000000 && addr < 0x11004000)
        {
            vu0->write_code<T>(addr - 0x11000000, data);
            return;
        }
        else if (addr >= 0x11004000 && addr < 0x11008000)
        {
            vu0->write_data<T>(addr - 0x11004000, data);
            return;
        }
        else if (addr >= 0x11008000 && addr < 0x1100C000)
        {
            vu1->write_code<T>(addr - 0x11008000, data);
            return;
        }
        else if (addr >= 0x1100C000 && addr < 0x11010000)
        {
            vu1->write_data<T>(addr - 0x1100C000, data);
            return;
        }
        else if (addr >= 0x10003c00 && addr <= 0x10003ce0)
        {
            vif1->write(addr, data);
            return;
        }
        else if (addr >= 0x10003800 && addr <= 0x10003890)
        {
            vif0->write(addr, data);
            return;
        }
        else if (addr == 0x10004000)
        {
            vif0->write_fifo(data);
            return;
        }
        else if (addr == 0x10005000)
        {
            vif1->write_fifo(data);
            return;
        }
        else if (addr >= 0x10003000 && addr <= 0x10006000)
        {
            uint128_t i;
            i.u128 = (__uint128_t)data;
            gif->write32(addr, i);
            return;
        }
        else if (addr >= 0x10002000 && addr <= 0x10002030)
            return;
        else if (addr >= 0x1000F200 && addr <= 0x1000F260)
        {
            sif->write(addr, data);
            return;
        }
        else if (addr >= 0x1C000000 && addr <= 0x1C200000)
        {
            *(T*)&iop_ram[addr - 0x1C000000] = data;
            return;
        }

        switch (addr)
        {
        case 0x1000f000:
            intc_stat = data;
            return;
        case 0x1000f010:
            intc_mask = data;
            return;
        case 0x10007010:
            // misc SIO settings
        case 0x1000f100:
        case 0x1000f120:
        case 0x1000f140:
        case 0x1000f150:
        case 0x1000f500:
        case 0x1000f400:
        case 0x1000f410:
        case 0x1000f420:
        case 0x1000f450:
        case 0x1000f460:
        case 0x1000f470:
        case 0x1000f480:
        case 0x1000f490:
        case 0x1000f510:
            return;
        case 0x1000f180:
            console << (char)data;
            console.flush();
            return;
        case 0x1000f430:
        {
            uint8_t SA = (data >> 16) & 0xFFF;
            uint8_t SBC = (data >> 6) & 0xF;

            if (SA == 0x21 && SBC == 0x1 && ((MCH_DRD >> 7) & 1) == 0)
                rdram_sdevid = 0;

            MCH_RICM = data & ~0x80000000;
            return;
        }
        case 0x1000f440:
            MCH_DRD = data;
            return;
        case 0x1a000008:
        case 0x1f80141c:
        case 0x1f801470:
        case 0x1f801472:
            return;
        }

        printf("[emu/Bus]: %s: Write %ld to unknown addr 0x%08x\n", __FUNCTION__, sizeof(T), addr);
        exit(1);
    }

    VectorUnit* GetVU0() {return vu0;}
    SubsystemInterface* GetSif() {return sif;}
    IoProcessor* GetIop() {return iop;}
    IopBus* GetBus() {return iop_bus;}

    void Dump();
};