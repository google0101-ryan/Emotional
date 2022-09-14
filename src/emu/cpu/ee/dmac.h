#pragma once

#include <util/uint128.h>

class EmotionDma
{
private:
    struct CHCR
    {
        struct
        {
            uint32_t dir : 1;
            uint32_t unused0: 1;
            uint32_t mode : 2;
            uint32_t asp : 2;
            uint32_t transfer_dmatag : 1;
            uint32_t irq_en : 1;
            uint32_t start : 1;
            uint32_t unused1 : 7;
            uint32_t tag : 16;
        };
        uint32_t value;
    };

    struct Channel
    {
        CHCR chcr;
        uint32_t madr;
        uint32_t tadr;
        uint32_t qwords;
        uint32_t save_tag_addr;
        uint32_t scratchpad_addr;
        uint32_t asr0;
        uint32_t asr1;
    } channels[10];
    
    union
    {
        uint32_t value;
        struct
        {
            uint32_t status : 10;
            uint32_t unused0 : 3;
            uint32_t stall_int : 1;
            uint32_t mfifo_empty : 1;
            uint32_t buserr_int : 1;
            uint32_t int_mask : 10;
            uint32_t unused1 : 3;
            uint32_t stall_int_mask : 1;
            uint32_t mfifo_empty_mask : 1;
        };
    } stat;

    union
    {
        uint32_t value;
        union
        {
            uint32_t dma_enable : 1;
            uint32_t cycle_stealing : 1;
            uint32_t mfifo_drain : 2;
            uint32_t stall_chan : 2;
            uint32_t stall_drain : 2;
            uint32_t release_cycle : 3;
        };
    } ctrl;

    uint32_t prio;
    uint32_t mfifo_size;
    uint32_t mfifo_addr;

    enum CHANNELS
    {
        VIF0,
        VIF1,
        GIF,
        IPU_FROM,
        IPU_TO,
        SIF0,
        SIF1,
        SIF2,
        SPR_FROM,
        SPR_TO
    };
public:
    void tick(int cycles);

    void write(uint32_t addr, uint32_t data);
    void write_dma(uint32_t addr, uint32_t data);

    uint32_t read(uint32_t addr);

    uint32_t read_dma(uint32_t addr);
};