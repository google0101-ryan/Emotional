#pragma once

#define VIF_NOP 0
#define VIF_STCYCL(wl, cl) ((1 << 24) | ((wl & 0xff) << 8) | ((cl & 0xff)))
#define VIF_OFFSET(offs) ((2 << 24) | (offs&0x3FF))
#define VIF_BASE(base) ((3 << 24) | (base&0x3FF))
#define VIF_ITOP(itop) ((4 << 24) | (itop&0x3FF))
#define VIF_STMOD(mode) ((5 << 24) | (mode & 3))
#define VIF_MSKPATH3(mask) ((6 << 24) | ((mask & 1) << 15))
#define VIF_MARK(mark) ((7 << 24) | (mark&0xffff))
#define VIF_STCOL (0x31 << 24)