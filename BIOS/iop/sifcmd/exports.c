#include <sifcmd/global.h>
#include <sifman/global.h>
#include <intrman/global.h>
#include <stdlib.h>

void sceSifCmdInit()
{
	// Send SIFCMD initialized to EE
	sceSetSMFLAG(0x20000);
	// TODO: Some kind of event wait happens here
}

int sifRpcInited = 0;

typedef struct SifCmdHandler
{
	void* func;
	void* harg;
} SifCmdHandler;

SifCmdHandler handlers[32];
char dmaDest[32];

void RegisterCommandHandler(unsigned int cid, void* func, void* harg)
{
	handlers[cid].func = func;
	handlers[cid].harg = harg;
}

unsigned int sceSifSendCmd_Internal(unsigned int cmd, int disableIntrs, SifCmdHdr* packet, int packet_size, void *src_extra,void *dest_extra, int size_extra)
{
	SifDmaTransfer transfers[2];
	int disableIntrs;

	int ret = 1;

	if (packet_size - 0x10 < 0x61)
	{
		if (size_extra < 1)
		{
			packet->dest = NULL;
			packet->size &= 0xff;
		}
		else
		{
			transfers[0].src = src_extra;
			transfers[0].size = size_extra;
			transfers[0].attr = 0;
			transfers[0].dst = dest_extra;
		}
		int index = (size_extra != 0);
		packet->size = packet_size;
		packet->cid = cmd;
		transfers[index].src = packet;
		transfers[index].attr = 4;
		transfers[index].size = packet_size;
		transfers[index].dst = dmaDest;

		ret = disableIntrs;

		if (disableIntrs)
		{
			CpuSuspendIntr(&disableIntrs);
			sceQueueTransfer(transfers, index+1);
			CpuResumeIntr(disableIntrs);
		}
		else
			sceQueueTransfer(transfers, index+1);
	}
	else
		ret = 0;
	return ret;
}

unsigned int sceSifSendCmd(unsigned int cmd, void* packet, int packet_size, void *src_extra,void *dest_extra, int size_extra)
{
	sceSifSendCmd_Internal(cmd, 0, packet, packet_size, src_extra, dest_extra, size_extra);
}

void sceSifRpcInit(int mode)
{
	int intr;

	sceSifCmdInit();
	CpuSuspendIntr(&intr);

	if (!sifRpcInited)
	{
		sifRpcInited = 1;
		sceSifSendCmd(0x80000001, (void*)0x1a40, 0x18, NULL, NULL, 0);
	}
	CpuResumeIntr(intr);
}