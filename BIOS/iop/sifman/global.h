#pragma once

typedef struct
{
	void* src;
	void* dst;
	int size;
	int attr;
} SifDmaTransfer;

extern void sifCmdInit();
extern int sceSifCheckInit();
extern void sceSetSMFLAG(unsigned int value);
int sceQueueTransfer(SifDmaTransfer* transfer, int count);