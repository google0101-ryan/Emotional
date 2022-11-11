#pragma once

#include <util/uint128.h>

class Bus;

class EmotionDma
{
private:
    const uint32_t DMATAG_END = 0x7;

	union TagAddr
	{
		uint32_t value;
		struct
		{
			uint32_t address : 30;
			uint32_t mem_select : 1;
		};
	};

	union DMATag
	{
		__uint128_t value;
		struct
		{
			__uint128_t qwords : 16,
			: 10,
			priority : 2,
			id : 3,
			irq : 1,
			address : 31,
			mem_select : 1,
			data : 64;
		};
	};

	union DCHCR
	{
		uint32_t value;
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
	};

	struct DMACChannel
	{
		DCHCR control;
		uint32_t address;
		uint32_t qword_count;
		TagAddr tag_address;
		TagAddr saved_tag_address[2];
		uint32_t padding[2];
		uint32_t scratchpad_address;
		bool end_transfer = false;
	};

	union DSTAT
	{
		uint32_t value;
		struct
		{
			uint32_t channel_irq : 10; /* Clear with 1 */
			uint32_t : 3;
			uint32_t dma_stall : 1; /* Clear with 1 */
			uint32_t mfifo_empty : 1; /* Clear with 1 */
			uint32_t bus_error : 1; /* Clear with 1 */
			uint32_t channel_irq_mask : 10; /* Reverse with 1 */
			uint32_t : 3;
			uint32_t stall_irq_mask : 1; /* Reverse with 1 */
			uint32_t mfifo_irq_mask : 1; /* Reverse with 1 */
			uint32_t : 1;
		};
		/* If you notice above the lower 16bits are cleared when 1 is written to them
		   while the upper 16bits are reversed. So I'm making this struct to better
		   implement this behaviour */
		struct
		{
			uint32_t clear : 16;
			uint32_t reverse : 16;
		};
	};

	struct DMACGlobals
	{
		uint32_t d_ctrl;
		DSTAT d_stat;
		uint32_t d_pcr;
		uint32_t d_sqwc;
		uint32_t d_rbsr;
		uint32_t d_rbor;
		uint32_t d_stadr;
		uint32_t d_enable = 0x1201;
	};

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

	DMACChannel channels[10] = {};
	DMACGlobals globals = {};

    Bus* bus;
public:
    EmotionDma(Bus* bus);

    void tick(int cycles);

    void write(uint32_t addr, uint32_t data);
    void write_dma(uint32_t addr, uint32_t data);

    uint32_t read(uint32_t addr);

    uint32_t read_dma(uint32_t addr);
};