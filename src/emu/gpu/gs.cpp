// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include <emu/gpu/gs.h>
#include <emu/gpu/gs_types.h>
#include <emu/memory/Bus.h>

#include <cstdio>
#include <cstdlib>
#include <queue>
#include <cassert>
#include <fstream>
#include <SDL2/SDL.h>
#include <algorithm>
#include "gs.h"

namespace GS
{

GS_CSR csr;
GS_IMR imr;

PRIM prim;
RGBAQ rgbaq;
ST st;
UV uv;

uint8_t fog;
uint8_t scanmsk;

PRMODE prmode;
TEXCLUT texclut;
FOGCOL fogcol;
TEXA texa;

Bitbltbuf bitbltbuf;
TRXPOS trxpos;
TRXREG trxreg;
uint8_t trxdir;

SMODE2 smode2;

bool use_prim = false;
bool pabe = false;
bool dthe = false;
bool clamp = false;

XYZF xyzf2, xyzf3;

Context contexts[2], *cur_contex = &contexts[0];

uint8_t* vram, *drawBuf;

std::queue<Vertex> vertices;

int requiredVerts[] =
{
	1, // Point
	2, // Line
	2, // Line strip
	3, // Triangle
	3, // Triangle Strip
	3, // Triangle Fan
	2, // Sprite
	0, // Reserved
};

uint32_t ReadVram32(uint32_t base, uint32_t width, int32_t x, int32_t y, bool is_bitbltbuf = false)
{
	uint32_t addr = (is_bitbltbuf ? base*64 : base*2048) + x + (y * (width*64));
	return *(uint32_t*)&vram[addr*4];
}

void WriteVRAM32(uint32_t base, uint32_t width, int32_t x, int32_t y, uint32_t val, bool is_bitbltbuf = false)
{
	uint32_t addr = (is_bitbltbuf ? base*64 : base*2048) + x + (y * (width*64));
	*(uint32_t*)&vram[addr*4] = val;
}

void DrawPixel(int32_t x, int32_t y, uint32_t z, uint32_t color)
{
	auto& ctxt = contexts[use_prim ? prim.ctxt : prmode.ctxt];

	if (scanmsk == 2 && (y & 1) == 0)
		return;
	else if (scanmsk == 3 & (y & 1) == 1)
		return;

	if (x < ctxt.scissor.scax0 || x > ctxt.scissor.scax1 || y < ctxt.scissor.scay0 || y > ctxt.scissor.scay1)
		return;
	
	bool update_frame = true;
	bool update_alpha = true;
	bool update_z = !ctxt.zbuf.zmsk;

	uint8_t a = (color >> 24);

	if (ctxt.test.ate)
	{
		bool fail = false;
		switch (ctxt.test.atst)
		{
		case 0:
			fail = true;
			break;
		case 1:
			break;
		case 2:
			if (a >= ctxt.test.aref)
				fail = true;
			break;
		case 3:
			if (a > ctxt.test.aref)
				fail = true;
			break;
		case 4:
			if (a != ctxt.test.aref)
				fail = true;
			break;
		case 5:
			if (a < ctxt.test.aref)
				fail = true;
			break;
		case 6:
			if (a <= ctxt.test.aref)
				fail = true;
			break;
		case 7:
			if (a == ctxt.test.aref)
				fail = true;
			break;
		}

		if (fail)
		{
			switch (ctxt.test.afail)
			{
			case 0:
				return;
			case 1:
				update_z = false;
				break;
			case 2:
				update_frame = false;
				break;
			case 3:
				printf("Unhandled RGB_ONLY\n");
				exit(1);
			}
		}
	}

	bool pass_depth_test = true;

	if (ctxt.test.zte)
	{
		switch (ctxt.test.ztst)
		{
		case 0:
			pass_depth_test = false;
			break;
		case 1:
			pass_depth_test = true;
			break;
		case 2:
			pass_depth_test = (z >= ReadVram32(ctxt.zbuf.zbp, ctxt.frame.fbw, x, y));
			break;
		case 3:
			pass_depth_test = (z > ReadVram32(ctxt.zbuf.zbp, ctxt.frame.fbw, x, y));
			break;
		}
	}
	else
		update_z = false;

	if (!pass_depth_test)
		return;

	if (ctxt.test.date)
	{
		bool alpha = ReadVram32(ctxt.frame.fbp, ctxt.frame.fbw, x, y) & (1 << 31);
		if (ctxt.test.datm ^ alpha)
			return;
	}

	uint32_t mask = ctxt.frame.fbmsk;
	color = (color & ~mask) | (ReadVram32(ctxt.frame.fbp, ctxt.frame.fbw, x, y) & mask);
	
	if (update_frame)
	{
		WriteVRAM32(ctxt.frame.fbp, ctxt.frame.fbw, x, y, color);
	}

	if (update_z)
	{
		WriteVRAM32(ctxt.zbuf.zbp, ctxt.frame.fbw, x, y, z);
	}
}

void DrawSprite()
{
	cur_contex = &contexts[use_prim ? prim.ctxt : prmode.ctxt];
	auto ve0 = vertices.front();
	vertices.pop();
	auto ve1 = vertices.front();
	vertices.pop();
	
	int32_t x1, x2, y1, y2;
    int32_t u1, u2, v1, v2;
    uint8_t r1, r2, r3, g1, g2, g3, b1, b2, b3, a1, a2, a3;
	float s1, s2, t1, t2, q1, q2;
	x1 = ve1.vert.x - contexts[prim.ctxt].xyoffset.ofx;
	x2 = ve0.vert.x - contexts[prim.ctxt].xyoffset.ofx;
	y1 = ve1.vert.y - contexts[prim.ctxt].xyoffset.ofy;
	y2 = ve0.vert.y - contexts[prim.ctxt].xyoffset.ofy;
	u1 = ve1.uv.u;
	u2 = ve0.uv.v;
	v1 = ve1.uv.u;
	v2 = ve0.uv.v;
	uint32_t z = ve0.vert.z;

	auto vtx_color = ve0.rgba;

	if (x1 > x2)
	{
		std::swap(x1, x2);
		std::swap(y1, y2);
	}

	if (u1 > u2)
	{
		std::swap(u1, u2);
		std::swap(v1, v2);
	}

	printf("Sprite (%d, %d), (%d, %d), (%d, %d, %d, %d)\n", x1, y1, x2, y2, vtx_color.r, vtx_color.g, vtx_color.b, vtx_color.a);

	for (int32_t y = y1, v = v1; y < y2; y++, v++)
	{
		for (int32_t x = x1, u = u1; x < x2; x++, u++)
		{
			uint32_t final_color;
			if (!prim.tme)
				final_color = (vtx_color.a << 24) | (vtx_color.b << 16) | (vtx_color.g << 8) | vtx_color.r;
			else
			{
				uint32_t color = ReadVram32(cur_contex->tex0.tbp0, cur_contex->tex0.tbw, u, v, true);
				uint8_t r = color & 0xff;
				uint8_t g = (color >> 8) & 0xff;
				uint8_t b = (color >> 16) & 0xff;
				uint8_t a = (color >> 24) & 0xff;
				final_color = (a << 24) | (b << 16) | (g << 8) | r;
			}

			DrawPixel(x, y, z, final_color);
		}
	}
}

int32_t orient2d(Vector3& v1, Vector3& v2, Vector3& v3)
{
    return (v2.x - v1.x) * (v3.y - v1.y) - (v3.x - v1.x) * (v2.y - v1.y);
}

void DrawTriangle(Vertex p0, Vertex p1, Vertex p2)
{
	assert(!prim.tme);

	auto& ctxt = contexts[prim.ctxt];

	auto& v0 = p0.vert;
	auto& v1 = p1.vert;
	auto& v2 = p2.vert;

	v0.x -= ctxt.xyoffset.ofx;
	v1.x -= ctxt.xyoffset.ofx;
	v2.x -= ctxt.xyoffset.ofx;
	v0.y -= ctxt.xyoffset.ofy;
	v1.y -= ctxt.xyoffset.ofy;
	v2.y -= ctxt.xyoffset.ofy;

	if (orient2d(v0, v1, v2) < 0)
		std::swap(v1, v2);
	
	int32_t min_x = std::min({v0.x, v1.x, v2.x});
	int32_t max_x = std::max({v0.x, v1.x, v2.x});
	int32_t min_y = std::min({v0.y, v1.y, v2.y});
	int32_t max_y = std::max({v0.y, v1.y, v2.y});

	int32_t A12 = v0.y - v1.y;
    int32_t B12 = v1.x - v0.x;
    int32_t A23 = v1.y - v2.y;
    int32_t B23 = v2.x - v1.x;
    int32_t A31 = v2.y - v0.y;
    int32_t B31 = v0.x - v2.x;

    int r1 = p0.rgba.r;
    int r2 = p1.rgba.r;
    int r3 = p2.rgba.r;

    int g1 = p0.rgba.g;
    int g2 = p1.rgba.g;
    int g3 = p2.rgba.g;

    int b1 = p0.rgba.b;
    int b2 = p1.rgba.b;
    int b3 = p2.rgba.b;

	Vector3 min_corner;
	min_corner.x = min_x;
	min_corner.y = min_y;
	int32_t w1_row = orient2d(v1, v2, min_corner);
	int32_t w2_row = orient2d(v2, v0, min_corner);
	int32_t w3_row = orient2d(v0, v1, min_corner);

	int32_t divider = orient2d(v0, v1, v2);

	printf("Triangle (%d, %d), (%d, %d), (%d, %d)\n", v0.x, v0.y, v1.x, v1.y, v2.x, v2.y);

	for (int32_t y = min_y; y <= max_y; y++)
	{
		int32_t w1 = w1_row;
        int32_t w2 = w2_row;
        int32_t w3 = w3_row;

		for (int32_t x = min_x; x <= max_x; x++)
		{
			if ((w1 | w2 | w3) >= 0)
			{
				double z = (double) v0.z * w1 + (double) v1.z * w2 + (double) v2.z * w3;
				z /= divider;

				int r = ((float) r1 * w1 + (float) r2 * w2 + (float) r3 * w3) / divider;
				int g = ((float) g1 * w1 + (float) g2 * w2 + (float) g3 * w3) / divider;
				int b = ((float) b1 * w1 + (float) b2 * w2 + (float) b3 * w3) / divider;

				uint32_t color = (0xFF << 24) | (b << 16) | (g << 8) | r;
				DrawPixel(x, y, (uint32_t)z, color);
			}
			w1 += A23;
			w2 += A31;
			w3 += A12;
		}
		w1_row += B23;
		w2_row += B31;
		w3_row += B12;
	}
}

void AddVertex(uint64_t data, bool draw_kick = true)
{
	Vertex v;

	v.vert.x = data & 0xffff;
	v.vert.y = (data >> 16) & 0xffff;
	v.vert.z = (data >> 32) & 0xffffff;

	v.vert.x >>= 4;
	v.vert.y >>= 4;

	v.rgba.r = rgbaq.r;
	v.rgba.g = rgbaq.g;
	v.rgba.b = rgbaq.b;
	v.rgba.a = rgbaq.a;
	v.rgba.q = rgbaq.q;

	v.uv.u = uv.u;
	v.uv.v = uv.v;

	vertices.push(v);

	printf("%ld vertices out of %d (draw_kick: %d)\n", vertices.size(), requiredVerts[prim.prim], draw_kick);

	if ((vertices.size() == requiredVerts[prim.prim]) && draw_kick)
	{
		printf("%d\n", prim.prim);
		switch (prim.prim)
		{
		case 0:
		{
			v = vertices.front();
			uint32_t color = (0xFF << 24) | (v.rgba.b << 16) | (v.rgba.g << 8) | v.rgba.r;
			DrawPixel(v.vert.x, v.vert.y, v.vert.z, color);
			printf("Point at (%d, %d)\n", v.vert.x, v.vert.y);
			vertices.pop();
			break;
		}
		case 3:
		{
			auto& v1 = vertices.front();
			vertices.pop();
			auto& v2 = vertices.front();
			vertices.pop();
			auto& v3 = vertices.front();
			vertices.pop();
			DrawTriangle(v1, v2, v3);
			break;
		}
		case 6:
			DrawSprite();
			break;
		default:
			printf("Unknown shape type %d\n", prim.prim);
			exit(1);
		}
	}
}

uint64_t trxpos_dsax, trxpos_dsay;
uint64_t trxpos_ssax, trxpos_ssay;
uint64_t pixels_transferred = 0;

void DoVramToVram()
{
	printf("[emu/GS]: Local to local transfer, (%d, %d) -> (%d, %d) (%dx%d)\n", trxpos.ssax, trxpos.ssay, trxpos.dsax, trxpos.dsay, trxreg.rrw, trxreg.rrh);
	
	int max_pixels = trxreg.rrw * trxreg.rrh;
	int int_dest_x, int_source_x;
	int int_dest_y, int_source_y;

	uint16_t dest_start_x = 0, src_start_x = 0;
	int x_step = 0, y_step = 0;

	printf("[emu/GS]: Transmission direction %d\n", trxpos.dir);
	assert(!trxpos.dir);

	dest_start_x = int_dest_x = trxpos.dsax;
	src_start_x = int_source_x = trxpos.ssax;
	int_dest_y = trxpos.dsay;
	int_source_y = trxpos.ssay;
	x_step = 1;
	y_step = 1;

	while (pixels_transferred < max_pixels)
	{
		uint32_t data = ReadVram32(bitbltbuf.sbp, bitbltbuf.sbw, int_source_x, int_source_y, true);
		WriteVRAM32(bitbltbuf.dbp, bitbltbuf.dbw, int_dest_x, int_dest_y, data, true);

		pixels_transferred++;
		int_source_x += x_step;
		int_dest_x += x_step;

		if (pixels_transferred % trxreg.rrw == 0)
		{
			int_source_x = src_start_x;
			int_source_y += y_step;
			
			int_dest_x = dest_start_x;
			int_dest_y += y_step;
		}

		int_source_x %= 2048;
		int_source_y %= 2048;
		int_dest_x %= 2048;
		int_dest_y %= 2048;
	}

	pixels_transferred = 0;
	trxdir = 3;
}

void WriteRegister(uint64_t reg, uint64_t data)
{
	switch (reg)
	{
	case 0x00:
		printf("[emu/GS]: Write 0x%08lx to PRIM\n", prim.data);
		prim.data = data;
		printf("%ld, %d\n", prim.data, prim.prim);
		break;
	case 0x01:
		rgbaq.data = data;
		break;
	case 0x02:
		st.data = data;
		break;
	case 0x03:
		uv.data = data;
		printf("Wrote (%d, %d) to UV\n", uv.u, uv.v);
		break;
	case 0x05:
		AddVertex(data);
		break;
	case 0x06:
	{
		auto& tex0 = contexts[0].tex0;
		tex0.data = data;
		printf("TEX0_1 buffer is now at 0x%08x, %d bytes wide, format %d (%dx%d)\n", tex0.tbp0*64, tex0.tbw*64, tex0.psm, 1 << tex0.tw, 1 << tex0.th);
		break;
	}
	case 0x07:
	{
		auto& tex0 = contexts[1].tex0;
		contexts[1].tex0.data = data;
		printf("TEX0_1 buffer is now at 0x%08x, %d bytes wide, format %d (%dx%d)\n", tex0.tbp0*64, tex0.tbw*64, tex0.psm, 1 << tex0.tw, 1 << tex0.th);
		break;
	}
	case 0x08:
	case 0x09:
		break;
	case 0x0A:
		fog = (data >> 56) & 0xFF;
		break;
	case 0x14:
		contexts[0].tex1.data = data;
		break;
	case 0x15:
		contexts[1].tex1.data = data;
		break;
	case 0x16:
		contexts[0].tex2.data = data;
		break;
	case 0x17:
		contexts[1].tex2.data = data;
		break;
	case 0x18:
		contexts[0].xyoffset.data = data;
		break;
	case 0x19:
		contexts[1].xyoffset.data = data;
		break;
	case 0x1A:
		printf("[emu/GS]: Using %s\n", (data & 1) ? "prim" : "prmode");
		use_prim = data & 1;
		break;
	case 0x1B:
		prmode.data = data;
		printf("Writing 0x%08lx to PRMODE\n", data);
		break;
	case 0x1C:
		texclut.data = data;
		break;
	case 0x22:
		scanmsk = data & 0x3;
		break;
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
		break;
	case 0x3B:
		texa.data = data;
		break;
	case 0x3D:
		fogcol.data = data;
		break;
	case 0x3f:
		break;
	case 0x40:
		contexts[0].scissor.data = data;
		printf("Scissor_1 area is now (%d, %d), (%d, %d)\n", contexts[0].scissor.scax0, contexts[0].scissor.scay0, contexts[0].scissor.scax1, contexts[0].scissor.scay1);
		break;
	case 0x41:
		contexts[1].scissor.data = data;
		printf("Scissor_2 area is now (%d, %d), (%d, %d)\n", contexts[1].scissor.scax0, contexts[1].scissor.scay0, contexts[1].scissor.scax1, contexts[1].scissor.scay1);
		break;
	case 0x42:
	case 0x43:
	case 0x44:
		break;
	case 0x45:
		dthe = data & 1;
		break;
	case 0x46:
		clamp = data & 1;
		break;
	case 0x47:
		contexts[0].test.data = data;
		break;
	case 0x48:
		contexts[1].test.data = data;
		break;
	case 0x49:
		pabe = data & 1;
		break;
	case 0x4A:
	case 0x4B:
		break;
	case 0x4C:
		contexts[0].frame.data = data;
		printf("Framebuffer_1 is now at 0x%08x, %d pixels wide, format %d\n", contexts[0].frame.fbp*2048, contexts[0].frame.fbw*64, contexts[0].frame.psm);
		break;
	case 0x4D:
		contexts[1].frame.data = data;
		printf("Framebuffer_2 is now at 0x%08x, %d pixels wide, format %d\n", contexts[1].frame.fbp*2048, contexts[1].frame.fbw*64, contexts[1].frame.psm);
		break;
	case 0x4E:
		contexts[0].zbuf.data = data;
		printf("Zbuf_1 is now at 0x%08x, format %d (0x%08lx)\n", contexts[0].zbuf.zbp*2048, contexts[0].zbuf.psm, data);
		break;
	case 0x4F:
		contexts[1].zbuf.data = data;
		printf("Zbuf_2 is now at 0x%08x, format %d (0x%08lx)\n", contexts[1].zbuf.zbp*2048, contexts[1].zbuf.psm, data);
		break;
	case 0x50:
		printf("Writing 0x%08lx to bitbltbuf\n", data);
		bitbltbuf.value = data;
		break;
	case 0x51:
		trxpos.data = data;
		break;
	case 0x52:
		trxreg.data = data;
		break;
	case 0x53:
		trxdir = data & 0x3;
		trxpos_dsax = 0;
		trxpos_dsay = 0;
		trxpos_ssax = 0;
		trxpos_ssay = 0;

		printf("Doing transmission, direction %d, (%d, %d) -> (%d, %d) (%dx%d)\n", trxdir, trxpos.ssax, trxpos.ssay, trxpos.dsax, trxpos.dsay, trxreg.rrw, trxreg.rrh);

		if (trxdir == 2)
		{
			DoVramToVram();
		}

		break;
	case 0x61:
		csr.finish = 1;
		Bus::TriggerEEInterrupt(0);
		break;
	default:
		printf("Write to unknown GS register 0x%02lx\n", reg);
		exit(1);
	}
}

void WriteHWReg(uint64_t data)
{
	auto write_pixel = [=](uint32_t data) {
		uint32_t x = (trxpos_dsax + trxpos.dsax) & 0x7FF;
		uint32_t y = (trxpos_dsay + trxpos.dsay) & 0x7FF;
		WriteVRAM32(bitbltbuf.dbp, bitbltbuf.dbw, x, y, data, true);
		trxpos_dsax++;
		if (trxpos_dsax >= trxreg.rrw)
		{
			trxpos_dsax = 0;
			trxpos_dsay++;
		}
	};

	write_pixel(data & 0xffffffff);
	write_pixel(data >> 32);

	if (trxpos_dsay >= trxreg.rrh)
		trxdir = 3;
}

void WriteRGBAQ(uint8_t r, uint8_t g, uint8_t b, uint8_t a, float q)
{
	printf("Writing (%d, %d, %d, %d, %f)\n", r, g, b, a, q);
	rgbaq.r = r;
	rgbaq.g = g;
	rgbaq.b = b;
	rgbaq.a = a;
	rgbaq.q = q;
}

void WriteXYZF(int32_t x, int32_t y, int32_t z, uint8_t f, bool adc)
{
	auto& xyzf = adc ? xyzf3 : xyzf2;

	x >>= 4;
	y >>= 4;

	xyzf.x = x;
	xyzf.y = y;
	xyzf.z = z;
	xyzf.f = f;

	AddVertex(xyzf.data, !adc);
}

void WriteGSSMODE2(uint64_t data)
{
	smode2.data = data;
}

SDL_Window *window;
SDL_Renderer* renderer;
SDL_Texture* texture;

void Initialize()
{
	vram = new uint8_t[4*1024*1024];
	drawBuf = new uint8_t[4*1024*1024];

	SDL_Init(SDL_INIT_EVERYTHING);

	window = SDL_CreateWindow("Emotional - PS2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 960, 0);
	renderer = SDL_CreateRenderer(window, -1, -1);
	// SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR32, SDL_TEXTUREACCESS_STATIC, 640, 224);	
}

void DumpVram()
{
	std::ofstream out("vram.bin");

	out.write((char*)vram, 4*1024*1024);
	out.close();
	
	out.open("disp.bin");

	out.write((char*)drawBuf, 4*1024*1024);
	out.close();

	out.open("tex0.bin");

	for (int y = 0; y < (1 << contexts[0].tex0.th); y++)
	{
		for (int x = 0; x < (1 << contexts[0].tex0.tw); x++)
		{
			uint32_t color = ReadVram32(contexts[0].tex0.tbp0, contexts[0].tex0.tbw, x, y, true);
			out.write((char*)&color, 4);
		}
	}

	out.close();
}

bool is_in_frame(DISPLAY& display, int32_t x_start, int32_t y_start, int32_t x, int32_t y)
{
	int width = contexts[1].display.dw+1;
	width /= contexts[1].display.magh+1;
	if (x >= x_start && x < (x_start + width))
	{
		if (y >= y_start && y < (y_start + contexts[1].display.dh+1))
		{
			return true;
		}
	}
	return false;
}

void render_frame()
{
	uint32_t* target = (uint32_t*)drawBuf;
	int32_t width;
	int32_t height;
	int32_t y_increment = 1;
	int32_t start_scanline = 0;
	int32_t frame_line_increment = 1;
	int32_t fb_offset = 0;
	int32_t display1_yoffset = 0;
	int32_t display1_xoffset = 0;
	int32_t display2_yoffset = 0;
	int32_t display2_xoffset = 0;
	bool enable_circuit1 = false;
	bool enable_circuit2 = true;
	bool field_offset = !csr.field;
	
	width = contexts[1].display.dw+1;
	width /= contexts[1].display.magh+1;
	height = contexts[1].display.dh+1;

	if (height >= (width * 1.3))
		height = height / 2;

	memset((uint8_t*)drawBuf, 0, 4*1024*1024);

	for (int y = start_scanline; y < height; y += y_increment)
	{
		for (int x = 0; x < width; x++)
        {
			int32_t pixel_x = x;
            int32_t pixel_y = y;

			if (!is_in_frame(contexts[1].display, display2_xoffset, display2_yoffset, pixel_x, pixel_y))
				continue;

			uint32_t output2 = ReadVram32(contexts[1].dispfb.fbp, contexts[1].dispfb.fbw, x, y);

			uint8_t r = output2 & 0xff;
			uint8_t g = (output2 >> 8) & 0xff;
			uint8_t b = (output2 >> 16) & 0xff;
			uint8_t a = (output2 >> 24) & 0xff;

			uint32_t final_color = r | (g << 8) | (b << 16) | (a << 24);

			if (smode2.interlace)
			{
				if (smode2.ffmd)
				{
					pixel_y *= 2;
					target[pixel_x + (pixel_y*width)] = final_color;
					target[pixel_x + ((csr.field ? pixel_y+1 : pixel_y-1)*width)] = final_color;
				}
				else
					target[pixel_x + (pixel_y*width)] = final_color;
			}
			else
				target[pixel_x + (pixel_y*width)] = final_color;
		}
	}
}

void SetVblankStart(bool start)
{
	csr.vsint = start;

	if (start)
	{		
		int width = contexts[1].display.dw+1;
		width /= contexts[1].display.magh+1;
		int height = contexts[1].display.dh+1;

		render_frame();

		SDL_Surface* sur = SDL_CreateRGBSurfaceFrom(drawBuf, width, height, 32, width*4, 0xFF, 0xFF00, 0xFF0000, 0xFF000000);

		SDL_Surface* winSur = SDL_GetWindowSurface(window);

		SDL_BlitScaled(sur, NULL, winSur, NULL);
		SDL_UpdateWindowSurface(window);

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				SDL_Quit();
				exit(0);
			}
		}
	}
}

void SetHblank(bool hb)
{
	csr.hsint = hb;
}

void UpdateOddFrame()
{
	csr.field = !csr.field;
}

void WriteGSCSR(uint64_t data)
{
	csr.signal &= ~(data & 1);
	if (data & 2)
	{
		csr.finish = false;
	}
	if (data & 4)
	{
		csr.hsint = false;
	}
	if (data & 8)
	{
		csr.vsint = false;
	}
	if (data & 0x200)
	{
		csr.data = 0;
		csr.fifo = 0x1;
		printf("[emu/GS]: Reset\n");
	}
}

uint64_t ReadGSCSR()
{
	uint64_t reg = csr.data;

    reg |= 0x1B << 16;
    reg |= 0x55 << 24;

	return reg;
}

void WriteIMR(uint64_t data)
{
	imr.value = data;
}

bool VSIntEnabled()
{
    return !imr.vsmsk;
}

bool HSIntEnabled()
{
    return !imr.hsmsk;
}

void WritePRIM(uint64_t data)
{
	printf("Write 0x%08lx to prim\n", data);
	prim.data = data;
}

void UpdateFPS(double fps)
{
	static char buf[256];

	int width = contexts[1].display.dw+1;
	width /= contexts[1].display.magh+1;
	int height = contexts[1].display.dh+1;

	sprintf(buf, "Emotional - PS2 Emulator: %dx%d, %f fps\n", width, height, fps);
	SDL_SetWindowTitle(window, buf);
}

void WriteBGCOLOR(uint64_t data)
{
}

void WriteDISPFB1(uint64_t data)
{
	auto& dispfb = contexts[0].dispfb;
	dispfb.value = data;
	printf("Area 1 read buffer is now at 0x%08x, %d pixels wide, format %d\n", dispfb.fbp*2048, dispfb.fbw*64, dispfb.psm);
}

void WriteDISPLAY1(uint64_t data)
{
	auto& display = contexts[0].display;
	contexts[0].display.value = data;

	int width = contexts[1].display.dw+1;
	width /= contexts[1].display.magh+1;
	int height = contexts[0].display.dh+1;

	printf("DISPLAY1 area is now %dx%d\n", width, height);
}

void WriteDISPFB2(uint64_t data)
{
	auto& dispfb = contexts[1].dispfb;
	dispfb.value = data;
	printf("Area 2 read buffer is now at 0x%08x, %d pixels wide, format %d\n", dispfb.fbp*2048, dispfb.fbw*64, dispfb.psm);
}

void WriteDISPLAY2(uint64_t data)
{
	auto& display = contexts[1].display;
	contexts[1].display.value = data;

	int width = contexts[1].display.dw+1;
	width /= contexts[1].display.magh+1;
	int height = contexts[1].display.dh+1;

	printf("DISPLAY2 area is now %dx%d (0x%08lx)\n", width, height, data);

	// if (contexts[1].display.dh != 0 && contexts[1].display.dw != 0)
	// {
	// 	SDL_DestroyRenderer(renderer);
	// 	SDL_DestroyWindow(window);

	// 	window = SDL_CreateWindow("Emotional - PS2 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, 0);
	// 	renderer = SDL_CreateRenderer(window, -1, -1);
	// }
}
} // namespace GS
