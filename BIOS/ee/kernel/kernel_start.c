#include <stdint.h>
#include <stdio.h>
#include "hw.h"
#include "reset.h"
#include "vif1.h"

// TODO: Split this into multiple files to clean up a bit

// TODO: This sets up a bunch of virtual -> physical mappings
long InitTLB()
{
	int kernelBitPos = 0xD + 0x12;
	printf("# TLB spad=0 kernel=1:%d default=%d:%d extended=%d:%d\n", kernelBitPos-1, kernelBitPos, kernelBitPos+0x7);
}

void ClearInterrupt(int index)
{
	// Clear all pending interrupts + index, except SBUS and Timer 3 interrupts
	INTC_MASK = INTC_MASK & index & 0xdffd;
	INTC_STAT = INTC_STAT & index & 0xdffd;
}

uint32_t GsInitRegList[] =
{
	0x1A,
	0x1B,
	0x0A,
	0x01,
	0x02,
	0x03,
	0x18,
	0x19,
	0x22,
	0x06,
	0x07,
	0x14,
	0x15,
	0x16,
	0x17,
	0x1C,
	0x34,
	0x35,
	0x36,
	0x37,
	0x3B,
	0x08,
	0x09,
	0x3D,
	0x40,
	0x41,
	0x47,
	0x48,
	0x42,
	0x43,
	0x49,
	0x44
};

void InitGS()
{	
	// Reset GIF bit
	GIF_CTRL = 1;
	// Reset GS bit
	GS_CSR = 0x200;


	// GIF Tag settings:
	// NLOOP: 0x2c
	// EOP: 1
	// PRIM enable: 0
	// PRIM data: 0
	// Data format: PACKED
	// NREGS: 1
	// Reg list: A+D
	GIF_FIFO0 = 0x802c;
	GIF_FIFO1 = 0x10000000;
	GIF_FIFO2 = 0xe;
	GIF_FIFO3 = 0;

	int listIndex = 0;

	int j = 0;
	for (int i = 0; i < 11; i++)
	{
		uint32_t* curRegListSlice = &GsInitRegList[i];
		j = 3;
		for (; j >= 0; j--)
		{
			// Register field
			// First 64 bits are the register data
			// Last 64 bits are the register
			// This is technically a 128-bit store
			GIF_FIFO0 = 0;
			GIF_FIFO1 = 0;
			GIF_FIFO2 = *curRegListSlice;
			GIF_FIFO3 = 0;
			curRegListSlice++;
		}
		// Wait for the GIF to finish processing this data
		// Read the GIF FIFO count and wait for it to be 0	
		while ((GIF_STAT & 0x1f000000) != 0)
			;
		j = i * 0x10;
	}

	// Ack any GS interrupts
	ClearInterrupt(1);
}

uint64_t GS_Settings = 0x90B27D0000000000ULL;

void SetGsCrt(int interlaced, int display_mode, int frame)
{
	if (display_mode == 0)
	{
		display_mode = 2;
		if ((((GS_Settings << 0x1d) >> 32) & 7) == 2)
			display_mode = 3;
	}

	int delay = 9999;
	do
	{
		delay--;
	} while (delay >= 0);

	if (display_mode == 2)
	{
		if (interlaced == 0)
		{
			unsigned int VHP = (((GS_Settings << 0x1f) >> 0x20 & 1) << 0x24);
			unsigned int GCONT = (GS_Settings & 1) << 0x19;
			GS_SMODE1 = GCONT | VHP | 0x740834504;
			// Horizontal front porch: 64
			// Horizontal back porch: 222
			// HSEQ: 124
			// HSVS: 1462
			// HS: 254
			GS_SYNCH1 = 0x7f5b61f06f040;
			// HB: 1652
			// HF: 1240
			GS_SYNCH2 = 0x33a4d8;
			// Vertical front porch: two halflines
			// Vertical front porch, no color burst: 6 halflines
			// Vertical back porch, color burst: 13 halflines
			// Vertical back porch, no color burst: 3 halflines
			// Half lines with video data: 240
			// Half lines with VSYNC: 3
			GS_SYNCV = 0xc7800601a01802;
			// Non-interlaced, field mode, power mode = 0 = ON
			GS_SMODE2 = 0;
			// Undocumented and unknown
			GS_SRFSH = 8;
			GS_SMODE1 = GCONT | 0x740814504;
			return;
		}
	}
}

int CheckAndSetINTMask(int index)
{
	int bit = 1 << (index & 0x1f);
	int ret = (INTC_MASK & bit) == 0;
	if (!ret)
		bit = INTC_MASK;
	INTC_MASK = bit;
	return ret;
}

void InitTimers()
{
	// Bus clock, gate disabled, do not clear counter, timer disabled, interrupts disabled, clear interrupts
	TIMER0_MODE = 0xc00;
	// Reset counter
	TIMER0_COUNT = 0;
	// Trigger interrupt just before overflow
	TIMER0_COMP = 0xffff;
	TIMER0_HOLD = 0;
	
	TIMER1_MODE = 0xc00;
	TIMER1_COUNT = 0;
	TIMER1_COMP = 0xffff;
	TIMER1_HOLD = 0;
	
	TIMER2_MODE = 0xc00;
	TIMER2_COUNT = 0;
	TIMER2_COMP = 0xffff;
	TIMER2_HOLD = 0;
	
	TIMER3_MODE = 0xc00;
	TIMER3_COUNT = 0;
	TIMER3_COMP = 0xffff;
	TIMER3_HOLD = 0;

	// Clear pending timer interrupts
	ClearInterrupt(0x1e00);
	// Unmask timer interrupts
	CheckAndSetINTMask(0xc);
}

uint32_t ChannelAddrList[] =
{
	0xB0008000, 0xB0009000, 0xB000A000, 0xB000B000,
	0xB000B400, 0xB000C000, 0xB000C400, 0xB000C800,
	0xB000D000, 0xB000D400
};

void ResetDMAC(int channel_mask)
{
	for (int i = 0; i < 11; i++)
	{
		int channelBit = i & 0x1f;
		if ((channel_mask >> channelBit) & 1)
		{
			uint32_t* addr = ChannelAddrList[i];
			addr[0x20] = 0;
			addr[0x00] = 0;
			addr[0x0C] = 0;
			addr[0x04] = 0;
			addr[0x14] = 0;
			addr[0x10] = 0;
		}
	}

	// Clear all newly initialized channel's interrupts
	// Also clear DMA stall, MFIFO empty, and BUSERR interrupts
	DMAC_STAT = channel_mask & 0xffff | 0xe000;
	int stat = DMAC_STAT;
	// If interrupts are enabled, disable them
	// 
	DMAC_STAT = stat & 0xffff0000 & (channel_mask | 0x6000);
	// If all channels are being reset and bits 5 & 6 are set, then also reset CTRL and PCR
	if ((channel_mask & 0x7f) == 0x7f)
	{
		DMAC_CTRL = 0;
		DMAC_PCR = 0;
	}
	else
	{
		// Enable DMA, and set all newly enabled channels to priority 0
		DMAC_CTRL = 1;
		int pcr = DMAC_PCR;
		DMAC_PCR = pcr & (~channel_mask << 16 | ~channel_mask);
	}

	DMAC_SQWC = 0;
	DMAC_RBSR = 0;
	DMAC_RBOR = 0;
}

void ClearVU0MemWord(int offs)
{
	// Write offs to VI1
	// Store VI0 to VI1
	// Restore VI1
}

void ClearVU0Mem128(int offs)
{
	// Save VI1
	// Save VF1
	// Zero out VF1
	// Set VI1 to offs
	// Zero 128-bits of memory at VI1
	// Restore VI1
	// Restore VF1
}

void ResetVU1()
{
	// Here, control VI12 has bit 0x200 set in VU0
	for (int i = 1; i < 0x10; i++)
	{
		ClearVU0MemWord(0x420 + i);
	}

	ClearVU0MemWord(0x434);
	ClearVU0MemWord(0x435);
	ClearVU0MemWord(0x436);
	ClearVU0MemWord(0x437);

	for (int i = 1; i < 0x20; i++)
	{
		ClearVU0Mem128(0x400 + i);
	}

	// Clear VU1 data memory
	for (int i = 0x3ff; i >= 0; i--)
	{
		VU1_DATA[i+0] = 0;
		VU1_DATA[i+1] = 0;
		VU1_DATA[i+2] = 0;
		VU1_DATA[i+3] = 0;
	}

	// Clear VU1 instruction memory
	uint64_t* memPtr = VU1_CODE;
	for (int i = 0x7ff; i >= 0; i--)
	{
		// Upper instruction: nop, lower instruction: ilw vi0, vi0(0x2FF)
		*memPtr = 0x8000033c080002ff;
		memPtr++;
	}

	// Clear VU1 interrupts
	ClearInterrupt(0x80);
}

void ResetVIF1()
{	
	// Reset VIF1
	VIF1_FBRST = 1;
	VIF1_STAT = 0;
	// Initialize VIF1 registers
	VIF1_FIFO0 = VIF_STCYCL(4, 4);
	VIF1_FIFO1 = VIF_STMOD(0);
	VIF1_FIFO2 = VIF_MSKPATH3(0);
	VIF1_FIFO3 = VIF_MARK(0);
	
	VIF1_FIFO0 = VIF_BASE(0);
	VIF1_FIFO1 = VIF_OFFSET(0);
	VIF1_FIFO2 = VIF_ITOP(0);
	VIF1_FIFO3 = VIF_NOP;
	
	VIF1_FIFO0 = VIF_OFFSET(0);
	VIF1_FIFO1 = VIF_NOP;
	VIF1_FIFO2 = VIF_BASE(0);
	VIF1_FIFO3 = VIF_NOP;
	
	VIF1_FIFO0 = VIF_NOP;
	VIF1_FIFO1 = VIF_NOP;
	VIF1_FIFO2 = VIF_NOP;
	VIF1_FIFO3 = VIF_STCOL;

	VIF1_FIFO0 = VIF_NOP;
	VIF1_FIFO1 = VIF_NOP;
	VIF1_FIFO2 = VIF_NOP;
	VIF1_FIFO3 = VIF_NOP;

	// Acknowledge VIF1 interrupts
	ClearInterrupt(0x20);
}

void ResetGIF()
{
	// Reset GIF
	GIF_CTRL = 1;
	// Clear GIF FIFO
	GIF_FIFO0 = 0;
	GIF_FIFO1 = 0;
	GIF_FIFO2 = 0;
	GIF_FIFO3 = 0;
}

void ResetVU0Watchdog()
{
	*(vu32*)0x1000F510 = 0;
	ClearInterrupt(0x4000);
}

void ClearVU0Mem()
{
	vu32* data = (vu32*)0x11000000;
	vu32* code = (vu32*)0x11004000;

	for (int i = 0xff; i >= 0; i--)
	{
		data[0] = 0;
		data[1] = 0;
		data[2] = 0;
		data[3] = 0;
		code[0] = 0;
		code[1] = 0;
		code[2] = 0;
		code[3] = 0;
	}
}

void ResetVU0Registers()
{
	// vsub vf0..31, vf0, vf0
	// viadd vi0..31, vi0, vi0
	// ctc2 zero,vc0
	// ctc2 zero,vc2
	// ctc2 zero,vc4
	// ctc2 zero,vc5
	// ctc2 zero,vc6
	// ctc2 zero,vc11
}

void ResetVU0()
{
	ResetVU0Watchdog();
	ClearVU0Mem();
	ResetVU0Registers();
	ClearInterrupt(0x40);
}

void ResetVIF0()
{
	VIF0_MARK = 0;
	VIF0_ERR = 0;
	VIF0_FBRST = 1;

	VIF0_FIFO0 = VIF_STCYCL(4, 4);
	VIF0_FIFO1 = VIF_OFFSET(0);
	VIF0_FIFO2 = VIF_NOP;
	VIF0_FIFO3 = VIF_STMOD(0);

	VIF0_FIFO0 = VIF_ITOP(0);
	VIF0_FIFO1 = VIF_NOP;
	VIF0_FIFO2 = VIF_NOP;
	VIF0_FIFO3 = VIF_NOP;
	ClearInterrupt(0x10);
}

void ResetEE(int mask)
{
	if (mask & RESET_DMAC)
	{
		printf("# Initialize DMAC ...\n");
		// Reset all channels
		ResetDMAC(0x31F);
	}
	if (mask & RESET_VU1)
	{
		printf("# Initialize VU1 ...\n");
		ResetVU1();
	}
	if (mask & RESET_VIF)
	{
		printf("# Initialize VIF1 ...\n");
		ResetVIF1();
	}
	if (mask & RESET_GIF)
	{
		printf("# Initialize GIF ...\n");
		ResetGIF();
	}
	if (mask & RESET_VU0)
	{
		printf("# Initialize VU0 ...\n");
		ResetVU0();
	}
	if (mask & RESET_VIF)
	{
		printf("# Initialize VIF0 ...\n");
		ResetVIF0();
	}
	// TODO: IPU Reset
	ClearInterrupt(0xc);
}

void ResetFPU()
{
	// Clear f0...f31
	// clear fcsr
}

uint32_t* GetUserMemEnd()
{

}

void ResetUserMem(uint32_t* begin)
{
	uint32_t* end = GetUserMemEnd();
	if (begin < end)
	{
		while (begin < end)
		{
			*begin = 0;
			begin[1] = 0;
			begin[2] = 0;
			begin[3] = 0;
			begin += 4;
		}
	}
}

void ClearScratchpad()
{
	uint32_t* sp = (uint32_t*)0x70000000;

	while (sp < (uint32_t*)0x70004000)
	{
		sp[0] = sp[1] = sp[2] = sp[3] = 0;
		sp += 4;
	}
}

void InitHardware()
{
	printf("Initialize Start.\n");
	printf("Initiailize GS ...");
	InitGS();
	SetGsCrt(1, GS_MODE_NTSC, 1);
	printf("\n");
	printf("# Initialize INTC ...\n");
	ClearInterrupt(0xffff);
	printf("# Initialize TIMER ...\n");
	InitTimers();
	ResetEE(RESET_ALL);
	printf("# Initialize FPU ...\n");
	ResetFPU();
	printf("# Initialize User Memory ...\n");
	ResetUserMem(0x80000);
	printf("# Initialize Scratch Pad ...\n");
	ClearScratchpad();
	printf("# Initialize Done.\n");
}

void main()
{
	InitTLB();
}