#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <app/opengl/common.h>
#include <app/opengl/renderer.h>

class GraphicsSynthesizer
{
public:
	union GS_CSR
	{
		uint64_t value;
		struct 
		{
			uint64_t signal : 1;
			uint64_t finish : 1;
			uint64_t hsint : 1;
			uint64_t vsint : 1;
			uint64_t edwint : 1;
			uint64_t zero : 2;
			uint64_t : 1;
			uint64_t flush : 1;
			uint64_t reset : 1;
			uint64_t : 2;
			uint64_t nfield : 1;
			uint64_t field : 1;
			uint64_t fifo : 2;
			uint64_t rev : 8;
			uint64_t id : 8;
			uint64_t : 32;
		};
	};

	struct GSPRegs
	{
		uint64_t pmode;
		uint64_t smode1;
		uint64_t smode2;
		uint64_t srfsh;
		uint64_t synch1;
		uint64_t synch2;
		uint64_t syncv;
		uint64_t dispfb1;
		uint64_t display1;
		uint64_t dispfb2;
		uint64_t display2;
		uint64_t extbuf;
		uint64_t extdata;
		uint64_t extwrite;
		uint64_t bgcolor;
		GS_CSR csr;
		uint64_t imr;
		uint64_t busdir;
		uint64_t siglblid;
	};

	union RGBAQReg
	{
		uint64_t value;
		struct
		{
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;
			float q;
		};
	};

	union XYZ
	{
		uint64_t value;
		struct
		{
			uint64_t x : 16;
			uint64_t y : 16;
			uint64_t z : 32;
		};
	};

	union XYZF
	{
		uint64_t value;
		struct
		{
			uint64_t x : 16;
			uint64_t y : 16;
			uint64_t z : 24;
			uint64_t f : 8;
		};
	};

	union XYOFFSET
	{
		uint64_t value;
		struct
		{
			uint16_t x_offset;
			uint16_t : 16;
			uint16_t y_offset;
		};
	};

	union BITBLTBUF
	{
		uint64_t value;
		struct
		{
			uint64_t source_base : 14;
			uint64_t : 2;
			uint64_t source_width : 6;
			uint64_t : 2;
			uint64_t source_pixel_format : 6;
			uint64_t : 2;
			uint64_t dest_base : 14;
			uint64_t : 2;
			uint64_t dest_width : 6;
			uint64_t : 2;
			uint64_t dest_pixel_format : 6;
		};
	};

	union TRXPOS
	{
		uint64_t value;
		struct
		{
			uint64_t source_top_left_x : 11;
			uint64_t : 5;
			uint64_t source_top_left_y : 11;
			uint64_t : 5;
			uint64_t dest_top_left_x : 11;
			uint64_t : 5;
			uint64_t dest_top_left_y : 11;
			uint64_t dir : 2;
		};
	};

	union TRXREG
	{
		uint64_t value;
		struct
		{
			uint64_t width : 12;
			uint64_t : 20;
			uint64_t height : 12;
		};
	};

	enum TRXDir : uint8_t
	{
		HostLocal = 0,
		LocalHost = 1,
		LocalLocal = 2,
		None = 3
	};

    union FRAME
    {
        uint64_t value;
        struct
        {
            uint64_t base_ptr : 9;
            uint64_t : 7;
            uint64_t width : 6;
            uint64_t : 2;
            uint64_t format : 6;
            uint64_t : 2;
            uint64_t drawing_mask : 32;
        };
    };

	uint64_t prim = 0;
	RGBAQReg rgbaq = {};
	uint64_t st = 0, uv = 0;
	XYZ xyz2 = {}, xyz3 = {};
	XYZF xyzf2 = {}, xyzf3 = {};
	uint64_t tex0[2] = {}, tex1[2] = {}, tex2[2] = {};
	uint64_t clamp[2] = {}, fog = 0, fogcol = 0;
	XYOFFSET xyoffset[2] = {};
	uint64_t prmodecont = 0, prmode = 0;
	uint64_t texclut = 0, scanmsk = 0;
	uint64_t miptbp1[2] = {}, miptbp2[2] = {};
	uint64_t texa = 0, texflush = 0;
	uint64_t scissor[2] = {}, alpha[2] = {};
	uint64_t dimx = 0, dthe = 0, colclamp = 0;
	uint64_t test[2] = {};
	uint64_t pabe = 0, fba[2];
    uint64_t zbuf[2] = {};
    FRAME frame[2] = {};
	BITBLTBUF bitbltbuf = {};
	TRXPOS trxpos = {};
	TRXREG trxreg = {};
	uint64_t trxdir = 0;

	int data_written = 0;

	GSPRegs priv_regs;

	std::queue<Vertex> vqueue = {};

	void submit_vertex(XYZ xyz, bool draw_kick);
	void submit_vertex_fog(XYZF xyzf, bool draw_kick);
	void process_vertex(Vertex vertex, bool draw_kick);

	Renderer* renderer;
public:
	GraphicsSynthesizer(Renderer* renderer);

	void write(uint16_t index, uint64_t data);

	uint64_t read32(uint32_t addr)
	{
		bool group = addr & 0xf000;
		uint32_t offset = ((addr >> 4) & 0xf) + 15 * group;
		auto ptr = (uint64_t*)&priv_regs + offset;

		return *ptr;
	}

    void write32(uint32_t addr, uint64_t data)
    {
		bool group = addr & 0xf000;
		uint32_t offset = ((addr >> 4) & 0xf) + 15 * group;
		auto ptr = (uint64_t*)&priv_regs + offset;

		*ptr = data;

		switch (offset)
		{
		case 15:
			if (data & 0x8)
				priv_regs.csr.vsint = false;
			break;
		}
    }

	GS_CSR& GetCSR()
	{
		return priv_regs.csr;
	}

	uint64_t GetIMR()
	{
		return priv_regs.imr;
	}
};