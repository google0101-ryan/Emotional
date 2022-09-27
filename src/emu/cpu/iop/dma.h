#pragma once

#include <util/uint128.h>

class IopBus;

class IoDma
{
private:
    union
    {
        uint32_t value;
        struct
        {
            uint32_t mdecin : 4,
            mdecout : 4,
            sif2 : 4,
            cdvd : 4,
            spu1 : 4,
            pio : 4,
            otc : 4,
            cpu : 4;
        };
    } dpcr;
    
    union
    {
        uint32_t value;
        struct
        {
            uint32_t spu2 : 4,
            dev9 : 4,
            sif0 : 4,
            sif1 : 4,
            sio2in : 4,
            sio2out : 4,
            usb : 4,
            unknown0 : 4;
        };
    } dpcr2;

    union
    {
        uint32_t value;
        struct
        {
            uint32_t completion_int : 7,
            unused0 : 8,
            err : 1,
            int_mask : 7,
            int_flag : 7,
            master_int : 1;
        };
    } dicr;

    union
    {
        uint32_t value;
        struct
        {
            uint32_t int_on_tag : 13,
            : 3,
            int_mask : 6,
            : 2,
            int_flag : 6,
            : 2;
        };
    } dicr2;

    struct Channel
    {
        uint32_t madr;
        uint32_t bcr;
        uint32_t chcr;
        uint32_t tadr;
    } channels[13];

    uint32_t glob_enable;
    IopBus* bus;
public:
    IoDma(IopBus* bus)
    : bus(bus) {}

    void tick(int cycles);

    void write(uint32_t addr, uint32_t data);
    uint32_t read(uint32_t addr);
};