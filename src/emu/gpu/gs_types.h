#pragma once

#include <cstdint>

namespace GS
{

union GS_CSR
{
	uint64_t data;
	struct
	{
		uint32_t signal : 1;
		uint32_t finish : 1;
		uint32_t hsint : 1;
		uint32_t vsint : 1;
		uint32_t edwint : 1;
		uint32_t : 3;
		uint32_t flush : 1;
		uint32_t reset : 1;
		uint32_t : 2;
		uint32_t nfield : 1;
		uint32_t field : 1;
		uint32_t fifo : 2;
		uint32_t rev : 8;
		uint32_t id : 8;
	};
};

union GS_IMR
{
	uint32_t value;
	struct
	{
		uint32_t : 8;
		uint32_t sigmsk : 1;
		uint32_t finishmsk : 1;
		uint32_t hsmsk : 1;
		uint32_t vsmsk : 1;
		uint32_t edwmsk : 1;
		uint32_t : 19;
	};
};

union DISPFB
{
	uint64_t value;
	struct
	{
		uint64_t fbp : 9;
		uint64_t fbw : 6;
		uint64_t psm : 5;
		uint64_t : 12;
		uint64_t dbx : 11;
		uint64_t dby : 11;
		uint64_t : 10;
	};
};

union DISPLAY
{
	uint64_t value;
	struct
	{
		uint64_t dx : 12;
		uint64_t dy : 11;
		uint64_t magh : 4;
		uint64_t magv : 2;
		uint64_t : 3;
		uint64_t dw : 12;
		uint64_t dh : 11;
		uint64_t : 9;
	};
};

union XYOFFSET
{
	uint64_t data;
	struct
	{
		uint64_t : 4;
		uint64_t ofx : 12;
		uint64_t : 20;
		uint64_t ofy : 12;
		uint64_t : 16;
	};
};

union TEX0
{
	uint64_t data;
	struct
	{
		uint64_t tbp0 : 14;
		uint64_t tbw : 6;
		uint64_t psm : 6;
		uint64_t tw : 4;
		uint64_t th : 4;
		uint64_t tcc : 1;
		uint64_t tfx : 2;
		uint64_t cbp : 14;
		uint64_t cpsm : 4;
		uint64_t csm : 1;
		uint64_t csa : 5;
		uint64_t cld : 3;
	};
};

union TEX1
{
	uint64_t data;
	struct
	{
		uint64_t lcm : 1;
		uint64_t : 1;
		uint64_t mxl : 3;
		uint64_t mmag : 1;
		uint64_t mmin : 3;
		uint64_t mtba : 1;
		uint64_t : 9;
		uint64_t l : 2;
		uint64_t : 4;
		uint64_t k : 7;
		uint64_t : 1;
		uint64_t : 20;
	};
};

union TEX2
{
	uint64_t data;
	struct
	{
		uint64_t : 20;
		uint64_t psm : 6;
		uint64_t : 11;
		uint64_t cbp : 14;
		uint64_t cpsm : 4;
		uint64_t csm : 1;
		uint64_t csa : 5;
		uint64_t cld : 3;
	};
};

union TEXCLUT
{
	uint64_t data;
	struct
	{
		uint64_t cbw : 6;
		uint64_t cou : 6;
		uint64_t cov : 10;
		uint64_t : 42;
	};
};

union SCISSOR
{
	uint64_t data;
	struct
	{
		uint64_t scax0 : 11;
		uint64_t : 5;
		uint64_t scax1 : 11;
		uint64_t : 5;
		uint64_t scay0 : 11;
		uint64_t : 5;
		uint64_t scay1 : 11;
		uint64_t : 5;
	};
};

union FRAME
{
	uint64_t data;
	struct
	{
		uint64_t fbp : 9;
		uint64_t : 7;
		uint64_t fbw : 6;
		uint64_t : 2;
		uint64_t psm : 6;
		uint64_t : 2;
		uint64_t fbmsk : 32;
	};
};

union ZBUF
{
	uint64_t data;
	struct
	{
		uint64_t zbp : 9;
		uint64_t : 15;
		uint64_t psm : 4;
		uint64_t : 4;
		uint64_t zmsk : 1;
		uint64_t : 31;
	};
};

union TEST
{
	uint64_t data;
	struct
	{
		uint64_t ate : 1;
		uint64_t atst : 3;
		uint64_t aref : 8;
		uint64_t afail : 2;
		uint64_t date : 1;
		uint64_t datm : 1;
		uint64_t zte : 1;
		uint64_t ztst : 2;
		uint64_t : 45;
	};
};

struct Context
{
	DISPFB dispfb;
	DISPLAY display;
	XYOFFSET xyoffset;
	TEX0 tex0;
	TEX1 tex1;
	TEX2 tex2;
	SCISSOR scissor;
	FRAME frame;
	ZBUF zbuf;
	TEST test;
};

union PRMODE
{
	uint64_t data;
	struct
	{
		uint64_t : 3;
		uint64_t iip : 1;
		uint64_t tme : 1;
		uint64_t fge : 1;
		uint64_t abe : 1;
		uint64_t aa1 : 1;
		uint64_t fst : 1;
		uint64_t ctxt : 1;
		uint64_t fix : 1;
		uint64_t : 53;
	};
};

union RGBAQ
{
	uint64_t data;
	struct
	{
		uint64_t r : 8;
		uint64_t g : 8;
		uint64_t b : 8;
		uint64_t a : 8;
		uint64_t q : 32;
	};
};

union ST
{
	uint64_t data;
	struct
	{
		float s;
		float t;
	};
};

union UV
{
	uint64_t data;
	struct
	{
		uint64_t : 4;
		uint64_t u : 10;
		uint64_t : 6;
		uint64_t v : 10;
		uint64_t : 34;
	};
};

union TEXA
{
	uint64_t data;
	struct
	{
		uint64_t ta0 : 8;
		uint64_t : 7;
		uint64_t aem : 1;
		uint64_t : 16;
		uint64_t ta1 : 8;
		uint64_t : 24;
	};
};

union FOGCOL
{
	uint64_t data;
	struct
	{
		uint64_t fcr : 8;
		uint64_t fcg : 8;
		uint64_t fcb : 8;
		uint64_t : 40;
	};
};

union Bitbltbuf
{
	uint64_t value;
	struct
	{
		uint64_t sbp : 14;
		uint64_t : 2;
		uint64_t sbw : 6;
		uint64_t : 2;
		uint64_t spsm : 6;
		uint64_t : 2;
		uint64_t dbp : 14;
		uint64_t : 2;
		uint64_t dbw : 6;
		uint64_t : 2;
		uint64_t dpsm : 6;
		uint64_t : 2;
	};
};

union TRXPOS
{
	uint64_t data;
	struct
	{
		uint64_t ssax : 11;
		uint64_t : 5;
		uint64_t ssay : 11;
		uint64_t : 5;
		uint64_t dsax : 11;
		uint64_t : 5;
		uint64_t dsay : 11;
		uint64_t dir : 2;
		uint64_t : 3;
	};
};

union TRXREG
{
	uint64_t data;
	struct
	{
		uint64_t rrw : 12;
		uint64_t : 20;
		uint64_t rrh : 12;
		uint64_t : 20;
	};
};

union PRIM
{
	uint64_t data;
	struct
	{
		uint64_t prim : 3;
		uint64_t iip : 1;
		uint64_t tme : 1;
		uint64_t fge : 1;
		uint64_t abe : 1;
		uint64_t aa1 : 1;
		uint64_t fst : 1;
		uint64_t ctxt : 1;
		uint64_t fix : 1;
		uint64_t : 53;
	};
};

union XYZF
{
	uint64_t data;
	struct
	{
		uint64_t : 4;
		uint64_t x : 12;
		uint64_t : 4;
		uint64_t y : 12;
		uint64_t z : 24;
		uint64_t f : 8;
	};
};

struct Vector3
{
	int32_t x, y;
	uint32_t z;
};

struct RGBAQ_Data
{
	uint32_t r, g, b, a;
	float q;
};

struct UV_Data
{
	uint32_t u, v;
};

struct Vertex
{
	Vector3 vert;
	RGBAQ_Data rgba;
	UV_Data uv;
};

union SMODE2
{
	uint64_t data;
	struct
	{
		uint64_t interlace : 1;
		uint64_t ffmd : 1;
		uint64_t dpms : 1;
		uint64_t : 61;
	};
};

}