#pragma once

#include <stdint.h>

typedef volatile uint32_t vu32;
typedef volatile uint64_t vu64;

#define GIF_CTRL (*(vu32*)0x10003000)
#define GIF_STAT (*(vu32*)0x10003020)
#define GIF_FIFO0 (*(vu32*)0x10006000)
#define GIF_FIFO1 (*(vu32*)0x10006004)
#define GIF_FIFO2 (*(vu32*)0x10006008)
#define GIF_FIFO3 (*(vu32*)0x1000600C)

#define DMAC_CTRL (*(vu32*)0x1000e000)
#define DMAC_STAT (*(vu32*)0x1000e010)
#define DMAC_PCR (*(vu32*)0x1000e020)
#define DMAC_SQWC (*(vu32*)0x1000e030)
#define DMAC_RBSR (*(vu32*)0x1000e040)
#define DMAC_RBOR (*(vu32*)0x1000e050)

#define GS_SMODE1 (*(vu64*)0x12000010)
#define GS_SYNCH1 (*(vu64*)0x12000040)
#define GS_SYNCH2 (*(vu64*)0x12000050)
#define GS_SYNCV (*(vu64*)0x12000060)
#define GS_SMODE2 (*(vu64*)0x12000020)
#define GS_SRFSH (*(vu64*)0x12000030)

#define INTC_STAT (*(vu32*)0xB000F000)
#define INTC_MASK (*(vu32*)0xB000F010)

#define TIMER0_COUNT (*(vu32*)0x10000000)
#define TIMER0_MODE (*(vu32*)0x10000010)
#define TIMER0_COMP (*(vu32*)0x10000020)
#define TIMER0_HOLD (*(vu32*)0x10000030)

#define TIMER1_COUNT (*(vu32*)0x10000800)
#define TIMER1_MODE (*(vu32*)0x10000810)
#define TIMER1_COMP (*(vu32*)0x10000820)
#define TIMER1_HOLD (*(vu32*)0x10000830)

#define TIMER2_COUNT (*(vu32*)0x10001000)
#define TIMER2_MODE (*(vu32*)0x10001010)
#define TIMER2_COMP (*(vu32*)0x10001020)
#define TIMER2_HOLD (*(vu32*)0x10001030)

#define TIMER3_COUNT (*(vu32*)0x10018000)
#define TIMER3_MODE (*(vu32*)0x10018010)
#define TIMER3_COMP (*(vu32*)0x10018020)
#define TIMER3_HOLD (*(vu32*)0x10018030)

#define VU1_CODE ((vu32*)0x11008000)
#define VU1_DATA ((vu32*)0x1100c000)

#define VIF1_STAT (*(vu32*)0x10003c00)
#define VIF1_FBRST (*(vu32*)0x10003c10)
#define VIF1_FIFO0 (*(vu32*)0x10005000)
#define VIF1_FIFO1 (*(vu32*)0x10005004)
#define VIF1_FIFO2 (*(vu32*)0x10005008)
#define VIF1_FIFO3 (*(vu32*)0x1000500C)

#define VIF0_FBRST (*(vu32*)0x10003810)
#define VIF0_ERR (*(vu32*)0x10003820)
#define VIF0_MARK (*(vu32*)0x10003830)
#define VIF0_FIFO0 (*(vu32*)0x10004000)
#define VIF0_FIFO1 (*(vu32*)0x10004004)
#define VIF0_FIFO2 (*(vu32*)0x10004008)
#define VIF0_FIFO3 (*(vu32*)0x1000400C)

#define GS_CSR (*(vu32*)0x12001000)

#define GS_MODE_NTSC 0x02