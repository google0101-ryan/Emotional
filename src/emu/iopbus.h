#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <emu/cpu/iop/dma.h>
#include <emu/dev/dvd.h>
#include <emu/cpu/iop/timers.h>
#include <emu/sif.h>
#include <fstream>

class Bus;

class IopBus
{
private:
    uint8_t* bios;
    uint8_t* ram;

    uint32_t i_mask;
    uint32_t i_ctrl;
    uint32_t i_stat;

    IoDma* dma;
    CDVD* dvd;
    SubsystemInterface* sif;
    Timer* timers;

    std::ofstream console;
public:
    uint32_t GetIMask() {return i_mask;}
    uint32_t GetICtrl() {return i_ctrl;}
    uint32_t GetIStat() {return i_stat;}

	uint8_t* GetRam() {return ram;}

    IopBus(uint8_t* pBios, uint8_t* pRam, SubsystemInterface* sif, Bus* parent)
    : bios(pBios), ram(pRam), sif(sif)
    {
        dma = new IoDma(this, parent);
        dvd = new CDVD();
        timers = new Timer(this);
        console.open("iop_log.txt");
        printf("0x%08x\n", *(uint32_t*)(bios + 0x2700));
        //exit(1);
    }

    IoDma* GetIopDma() {return dma;}
    Timer* GetIopTimers() {return timers;}
    SubsystemInterface* GetSif() {return sif;}

    void OutputToLog(char c)
    {
        console << c;
        console.flush();
    }

    template<typename T>
    T read(uint32_t addr)
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

        if (addr >= 0x1FC00000 && addr < 0x21C00000)
            return *(T*)(bios + (addr - 0x1FC00000));
        if (addr < 0x200000)
            return *(T*)&ram[addr];
        
        switch (addr)
        {
        case 0x1E000000 ... 0x1E03FFFF:
        case 0x1f80100C:
        case 0x1f801010:
        case 0x1f801450:
        case 0x1f801400:
            return 0;
        case 0x1F801080 ... 0x1F8010EC:
        case 0x1F801500 ... 0x1F80155C:
        case 0x1f8010f0:
        case 0x1f8010f4:
        case 0x1f801570:
        case 0x1f801574:
        case 0x1f801578:
            return dma->read(addr);
        case 0x1f402004 ... 0x1f402018:
            return dvd->read(addr);
        case 0x1f801100 ... 0x1f80112c:
        case 0x1f801480 ... 0x1f8014ac:
            return timers->read(addr);
        case 0x1f801074:
            return i_mask;
        case 0x1f801070:
            return i_stat;
        case 0x1f801078:
        {
            int ret = i_ctrl;
            i_ctrl = 0;
            return ret;
        }
        case 0x1D000010:
        case 0x1D000020:
        case 0x1D000030:
        case 0x1D000040:
        case 0x1D000060:
            return sif->read(addr);
        }

        printf("[emu/IopBus]: %s: Read from unknown addr 0x%08x\n", __FUNCTION__, addr);
        exit(1);
    }

    template<typename T>
    void write(uint32_t addr, T data)
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

        if (addr < 0x200000)
        {
            *(T*)&ram[addr] = data;
            return;
        }

        switch (addr)
        {
        case 0x1f801000:
        case 0x1f801004:
        case 0x1f801008:
        case 0x1f80100C:
        case 0x1f801010:
        case 0x1f801014:
        case 0x1f801018:
        case 0x1f80101C:
        case 0x1f801020:
        case 0x1f801060:
        case 0x1f801400:
        case 0x1f801404:
        case 0x1f801408:
        case 0x1f80140C:
        case 0x1f801410:
        case 0x1f801414:
        case 0x1f801418:
        case 0x1f80141C:
        case 0x1f801420:
        case 0x1f801450:
        case 0x1f8015f0:
        case 0x1F801560:
        case 0x1F801564:
        case 0x1F801568:
        case 0x1f802070:
        case 0x1ffe0130:
        case 0x1ffe0140:
        case 0x1ffe0144:
        case 0x1ffe0148:
        case 0x1ffe014C:
            return;
        case 0x1F801080 ... 0x1F8010EC:
        case 0x1F801500 ... 0x1F80155C:
        case 0x1F8010F0:
        case 0x1F8010F4:
        case 0x1F801570:
        case 0x1F801574:
        case 0x1F801578:
            dma->write(addr, data);
            return;
		case 0x1f801070:
			i_stat &= data;
			return;
        case 0x1f801074:
            i_mask = data;
            return;
        case 0x1f801078:
            i_ctrl = data;
            return;
        case 0x1D000010:
        case 0x1D000020:
        case 0x1D000030:
        case 0x1D000040:
            sif->write(addr, data);
            return;
        case 0x1f801100 ... 0x1f80112c:
        case 0x1f801480 ... 0x1f8014ac:
            timers->write(addr, data);
            return;
        }
        printf("[emu/IopBus]: %s: Write to unknown addr 0x%08x\n", __FUNCTION__, addr);
        exit(1);
    }

    void TriggerInterrupt(int cause)
    {
        i_stat |= (1 << cause);
    }
};