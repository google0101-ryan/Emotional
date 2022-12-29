#pragma once

#include <emu/cpu/ee/vu.h>
#include <queue>
#include <array>

// Used to upload compressed data to a VectorUnit via DMA
class VectorInterface
{
private:
	enum VIFCommands
	{
		NOP = 0x0,
		STCYCL = 0x1,
		OFFSET = 0x2,
		BASE = 0x3,
		ITOP = 0x4,
		STMOD = 0x5,
		MSKPATH3 = 0x6,
		MARK = 0x7,
		FLUSHE = 0x10,
		FLUSH = 0x11,
		FLUSHA = 0x13,
		MSCAL = 0x14,
		MSCALF = 0x15,
		MSCNT = 0x17,
		STMASK = 0x20,
		STROW = 0x30,
		STCOL = 0x31,
		MPG = 0x4a,
		DIRECT = 0x50,
		DIRECTHL = 0x51,
		UNPACKSTART = 0x60,
		UNPACKEND = 0x7f
	};

	union VIFSTAT
	{
		uint32_t value = 0;
		struct
		{
			uint32_t vif_status : 2;
			uint32_t e_wait : 1;
			uint32_t waiting_for_gif : 1;
			uint32_t : 2;
			uint32_t mark : 1;
			uint32_t double_buffer_flag : 1;
			uint32_t stalled_after_stop : 1;
			uint32_t stalled_after_break : 1;
			uint32_t stalled_on_intr : 1;
			uint32_t intr_detected : 1;
			uint32_t dmatag_mismatch : 1;
			uint32_t invalid_cmd : 1;
			uint32_t : 9;
			uint32_t fifo_detection : 1;
			uint32_t fifo_count : 5;
		};
	} stat;

	enum VIFUFormat
	{
		S_32 = 0,
		S_16 = 1,
		S_8 = 2,
		V2_32 = 4,
		V2_16 = 5,
		V2_8 = 6,
		V3_32 = 8,
		V3_16 = 9,
		V3_8 = 10,
		V4_32 = 12,
		V4_16 = 13,
		V4_8 = 14,
		V4_5 = 15
	};

	enum WriteMode : bool
	{
		Skipping = 0,
		Filling = 1,
	};

    VectorUnit* vu;
    int id;
    uint32_t fbrst; // Used to reset various states in the VIF
    uint32_t err; // Controls error and interrupt behavior
    uint32_t fifo_size; // Varies based on which VIF unit
    std::queue<uint32_t> fifo; // A first-in, first-out buffer

	void process_command();
	void execute_command();

	uint32_t mark = 0;
	uint32_t mode, num = 0;
	uint32_t mask = 0, code = 0, itops = 0;
	uint32_t base = 0, ofst = 0;
	uint32_t tops = 0, itop = 0, top = 0;
	std::array<uint32_t, 4> rn = {}, cn = {};

	WriteMode write_mode = WriteMode::Skipping;

	union VIFCommand
	{
		uint32_t value;
		struct
		{
			union
			{
				uint16_t immediate;
				struct
				{
					uint16_t address : 10;
					uint16_t : 4;
					uint16_t sign_extend : 1;
					uint16_t flg : 1;
				};
			};
			/* The NUM field stores the subpacket length. However depending on whether
			   the operation writes data or instructions to the VU its meaning can change.
			   In data transfer operations the field is in qwords while in instruction
			   transfers it's in dwords. */
			uint8_t num;
			union
			{
				uint8_t command;
				struct
				{
					uint8_t vl : 2;
					uint8_t vn : 2;
					uint8_t mask : 1;
				};
			};
		};
	} command = {};

	union VIFCYCLE
	{
		uint16_t value;
		struct
		{
			uint16_t cycle_length : 8;
			uint16_t write_cycle_length : 8;
		};
	} cycle;

	uint32_t subpacket_count = 0, address = 0;
	uint32_t qwords_written = 0, word_cycles = 0;
public:
    VectorInterface(VectorUnit* vu, int id) : vu(vu), id(id) 
    {
        if (id)
            fifo_size = 64;
        else
            fifo_size = 32;
    }

	void tick(int cycles);

    bool write_fifo(uint32_t data);
    bool write_fifo(__uint128_t data);

    void write(uint32_t addr, uint32_t data);
	uint32_t read(uint32_t addr);
};