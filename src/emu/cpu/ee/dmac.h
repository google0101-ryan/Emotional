#pragma once

#include <util/uint128.h>

class Bus;

class EmotionDma
{
private:
    union CHCR
    {
        struct
        {
            uint32_t direction : 1; /* 0 = to memory, 1 = from memory */
			uint32_t : 1;
			uint32_t mode : 2; /* 0 = normal, 1 = chain, 2 = interleave */
			uint32_t stack_ptr : 2;
			uint32_t transfer_tag : 1;
			uint32_t enable_irq_bit : 1;
			uint32_t running : 1;
			uint32_t : 7;
			uint32_t tag : 16;
        };
        uint32_t value;
    };

    union DMATag
    {
        uint64_t value;
        struct
        {
            uint64_t qword_count : 16,
            unused0 : 10,
            prio_control : 2,
            tag_id : 3,
            irq : 1,
            addr : 31,
            mem_select : 63;
        };
    };

    struct Channel
    {
        CHCR chcr;
        uint32_t addr;
        uint32_t qwords;
        uint32_t save_tag_addr;
        uint32_t scratchpad_addr;
        uint32_t asr0;
        uint32_t asr1;
        bool should_finish = false;
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

    Bus* bus;
public:
    EmotionDma(Bus* bus);

    void tick(int cycles);

    void write(uint32_t addr, uint32_t data);
    void write_dma(uint32_t addr, uint32_t data);

    uint32_t read(uint32_t addr);

    uint32_t read_dma(uint32_t addr);
};