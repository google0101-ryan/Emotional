#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <util/uint128.h>
#include <emu/gs/gs.h>

// Graphics InterFace, used for unpacking DMA transfers and uploading them
// to the GS
class GIF
{
private:
	union GIFTag
	{
		__uint128_t value;
		struct
		{
			__uint128_t nloop : 15;
			__uint128_t eop : 1;
			__uint128_t : 30;
			__uint128_t pre : 1;
			__uint128_t prim : 11;
			__uint128_t flg : 2;
			__uint128_t nreg : 4;
			__uint128_t regs : 64;
		};
	};


	int data_left = 0;
	int regs_left = 0;
	int regs_count = 0;
    uint32_t gif_ctrl;
    std::queue<uint128_t> fifo;
    GIFTag tag;

	float internal_Q;

	GraphicsSynthesizer* gpu;
public:
	GIF(GraphicsSynthesizer* gs);

    void tick(int cycles);
    void process_packed(uint128_t qword);
	void process_reglist(uint128_t qword);

    uint32_t read32(uint32_t addr)
    {
        switch (addr)
        {
        case 0x10003020:
            return (fifo.size() & 0x1F);
        default:
            printf("[emu/Gif]: %s: Read from unknown addr 0x%08x\n", __FUNCTION__, addr);
            exit(1);
        }
    }

    void write32(uint32_t addr, uint128_t data)
    {
        switch (addr)
        {
        case 0x10003000:
            gif_ctrl = data.u32[0];
            return;
        case 0x10006000:
        {
            fifo.push(data);
            return;
        }
        default:
            printf("[emu/Gif]: %s: Write to unknown addr 0x%08x\n", __FUNCTION__, addr);
            exit(1);
        }
    }
};