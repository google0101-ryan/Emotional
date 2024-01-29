#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <intrman/global.h>

typedef volatile uint32_t vu32;

#define DMA_DPCR (*(vu32*)0xbf8010f0)
#define DMA_DPCR2 (*(vu32*)0xbf801570)
#define DMA_SIF0_CTRL (*(vu32*)0xbf801528)
#define DMA_SIF1_CTRL (*(vu32*)0xbf801538)
#define DMA_SIF1_BCR (*(vu32*)0xbf801534)
#define DMA_SIF2_CTRL (*(vu32*)0xbf8010A8)
#define SIF_SMCOM (*(vu32*)0xbd000010)
#define SIF_MSFLG (*(vu32*)0xbd000020)
#define SIF_SMFLG (*(vu32*)0xbd000030)
#define SIF_CTRL (*(vu32*)0xbd000040)

bool sifCmdInited = false;
bool sif2Inited = false;

void sifInitSif2()
{
	if (!sif2Inited)
	{
		DMA_SIF2_CTRL = 0;
		DMA_DPCR |= 0x800;
		sif2Inited = true;
	}
}

uint32_t transferBufOffs = 0;
uint32_t* transferBufBase;
uint32_t curTransferId;

int HandleInterrupt(void* arg)
{

}

void InitInterrupts()
{
	int int_state;

	transferBufOffs = 0;
	transferBufBase = (uint32_t*)0xf8c; // TODO: This probably gets patched
	CpuSuspendIntr(&int_state);
	RegisterIntrHandler(0x2a, 1, HandleInterrupt, &curTransferId);
	EnableIntr(0x2a);
	CpuEnableIntr(int_state);
}

void SetupSIF1()
{
	if (!(SIF_CTRL & 0x40))
		SIF_CTRL = 0x40;
	DMA_SIF1_BCR = 0x20;
	// Sets up the mode and stuff, but leaves the channel disabled
	DMA_SIF1_CTRL = 0x41000300;
}

void sifCmdInit()
{
	if (!sifCmdInited)
	{
		int int_state;

		// This sets SIF0 and SIF1 to max priority
		DMA_DPCR2 = DMA_DPCR2 | 0x8800;
		DMA_SIF0_CTRL = 0;
		DMA_SIF1_CTRL = 0;

		// Setup SIF2. Afaik, this is exclusively used for PSX mode
		sifInitSif2();

		InitInterrupts();

		CpuSuspendIntr(&int_state);
		CpuEnableIntr();
		// Here, we poll MSFLG and check for bit 0x10000, which means the EE DMAC is ready
		uint32_t msflg;
		while (!(msflg & 0x10000))
		{
			int nested_int_disable;
			CpuSuspendIntr(&nested_int_disable);
			msflg = SIF_MSFLG;
			CpuResumeIntr(nested_int_disable);
		}
		CpuResumeIntr(int_state);

		SetupSIF1();

		// TODO: This might get patched on module load
		// Tell the EE where our recv buffer is
		SIF_SMCOM = 0;

		// Let the EE know that SIF has been initialized
		SIF_SMFLG = 0x10000;

		sifCmdInited = true;
	}
}