// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include <emu/cpu/ee/dmac.hpp>

#include <emu/memory/Bus.h>
#include <emu/sched/scheduler.h>
#include <emu/dev/sif.h>
#include <emu/gpu/gif.hpp>
#ifdef EE_JIT
#include <emu/cpu/ee/EmotionEngine.h>
#else
#include <emu/cpu/ee/ee_interpret.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include "dmac.hpp"

namespace DMAC
{


union DSTAT
{
	uint32_t value;
	struct
	{
		uint32_t channel_irq : 10; /* Clear with 1 */
		uint32_t : 3;
		uint32_t dma_stall : 1; /* Clear with 1 */
		uint32_t mfifo_empty : 1; /* Clear with 1 */
		uint32_t bus_error : 1; /* Clear with 1 */
		uint32_t channel_irq_mask : 10; /* Reverse with 1 */
		uint32_t : 3;
		uint32_t stall_irq_mask : 1; /* Reverse with 1 */
		uint32_t mfifo_irq_mask : 1; /* Reverse with 1 */
		uint32_t : 1;
	};
	/* If you notice above the lower 16bits are cleared when 1 is written to them
	   while the upper 16bits are reversed. So I'm making this struct to better
	   implement this behaviour */
	struct
	{
		uint32_t clear : 16;
		uint32_t reverse : 16;
	};
} stat;

union DN_SADR
{
    uint32_t data;
    struct
    {
        uint32_t address : 14;
        uint32_t : 17;
    };
};

union DN_CHCR
{
    uint32_t data;
    struct
    {
        uint32_t direction : 1;
        uint32_t : 1;
        uint32_t mode : 2;
        uint32_t asp : 2;
        uint32_t tte : 1;
        uint32_t tie : 1;
        uint32_t start : 1;
        uint32_t : 7;
        uint32_t tag : 16;
    };
};

struct Channel
{
    DN_SADR sadr;
    DN_CHCR chcr;
    uint32_t madr;
    uint32_t tadr;
	uint32_t qwc;
    uint32_t asr[2];
} channels[10];

uint32_t ctrl;
uint32_t d_enable = 0x1201;

union DMATag
{
	__uint128_t value;

	struct
	{
		__uint128_t qwc : 16;
		__uint128_t : 10;
		__uint128_t prio_control : 2;
		__uint128_t tag_id : 3;
		__uint128_t irq : 1;
		__uint128_t addr : 31;
		__uint128_t mem_sel : 1;
		__uint128_t data_transfer : 64;
	};
};

const char* REG_NAMES[] =
{
	"CHCR",
	"MADR",
	"QWC",
	"TADR",
	"ASR0",
	"ASR1",
	"",
	"",
	"",
	"SADR",
};

void WriteVIF0Channel(uint32_t addr, uint32_t data)
{
    switch (addr & 0xff)
    {
    case 0x00:
        channels[0].chcr.data = data;
        if (channels[0].chcr.start)
            printf("[emu/DMAC]: Starting VIF0 transfer\n");
        break;
    case 0x10:
        channels[0].madr = data  & 0x01fffff0;
        break;
    case 0x30:
        channels[0].tadr = data;
        break;
    case 0x40:
        channels[0].asr[0] = data;
        break;
    case 0x50:
        channels[0].asr[1] = data;
        break;
    case 0x80:
        channels[0].sadr.data = data;
        break;
    }
}

void WriteVIF1Channel(uint32_t addr, uint32_t data)
{
    switch (addr & 0xff)
    {
    case 0x00:
        channels[1].chcr.data = data;
        if (channels[1].chcr.start)
            printf("[emu/DMAC]: Starting VIF1 transfer\n");
        break;
    case 0x10:
        channels[1].madr = data & 0x01fffff0;
        break;
    case 0x30:
        channels[1].tadr = data;
        break;
    case 0x40:
        channels[1].asr[0] = data;
        break;
    case 0x50:
        channels[1].asr[1] = data;
        break;
    case 0x80:
        channels[1].sadr.data = data;
        break;
    }
}

void DoGIFTransfer()
{
    auto& c = channels[2];

    assert(c.chcr.mode == 0);

    while (c.qwc)
    {
        uint128_t data = Bus::Read128(c.madr);
        c.madr += 16;
        GIF::WriteFIFO(data);
        c.qwc--;
    }

    c.chcr.start = 0;
    
    stat.channel_irq |= (1 << 2);

#ifdef EE_JIT
	if (stat.channel_irq & stat.channel_irq_mask)
        EmotionEngine::SetIp1Pending();
#else
    if (stat.channel_irq & stat.channel_irq_mask)
        EEInterpreter::SetIP1Pending();
#endif
}

bool fetching_gif_tag = false;
bool gif_ongoing_transfer = false;
bool gif_irq_on_done = false;

DMATag gif_tag;

void DoGIFTransferChain()
{
    auto& c = channels[2];

	fetching_gif_tag = true;

	if (!gif_ongoing_transfer)
		return;
 
    if (c.qwc > 0)
	{
        while (c.qwc)
        {
            uint128_t data = Bus::Read128(c.madr);
            c.madr += 16;
            GIF::WriteFIFO(data);
            c.qwc--;

            printf("Writing %s to GIF FIFO\n", print_128(data));
        }
	}
	else if (gif_irq_on_done)
	{
		printf("[emu/DMAC]: Transfer ended on GIF channel\n");

		stat.channel_irq |= (1 << 2);

#ifdef EE_JIT
		if (stat.channel_irq & stat.channel_irq_mask)
            EmotionEngine::SetIp1Pending();
#endif

		c.chcr.start = 0;
        gif_ongoing_transfer = false;
        gif_irq_on_done = false;
	}
    else
    {
        auto address = c.tadr;

        gif_tag.value = Bus::Read128(address).u128;

        c.qwc = gif_tag.qwc;
        c.chcr.tag = (gif_tag.value >> 16) & 0xffff;

        uint16_t tag_id = gif_tag.tag_id;
        switch (tag_id)
        {
        case 0:
            gif_irq_on_done = true;
            c.madr = gif_tag.addr;
            c.tadr += 16;
            break;
        case 1:
            c.madr = c.tadr+16;
            c.tadr = c.madr+c.qwc*16;
            break;
        case 7:
            gif_irq_on_done = true;
            c.madr = c.tadr+16;
            break;
        default:
            printf("[emu/GIF]: Unknown tag id %d\n", tag_id);
            exit(1);
        }
    }

	if (gif_ongoing_transfer)
	{
		Scheduler::Event gif_evt;
		gif_evt.cycles_from_now = 2;
		gif_evt.func = DoGIFTransferChain;
		gif_evt.name = "GIF DMA transfer";

		Scheduler::ScheduleEvent(gif_evt);
	}
}

void WriteGIFChannel(uint32_t addr, uint32_t data)
{
    switch (addr & 0xff)
    {
    case 0x00:
        channels[2].chcr.data = data;
        if (channels[2].chcr.start)
        {
            Scheduler::Event gif_event;
            gif_event.cycles_from_now = channels[2].qwc*4;
            gif_event.func = (channels[2].chcr.mode == 0 ? DoGIFTransfer : DoGIFTransferChain);
            gif_event.name = "GIF DMAC transfer";
            Scheduler::ScheduleEvent(gif_event);
            printf("Starting GIF DMAC transfer (%d qwords)\n", channels[2].qwc);
            gif_ongoing_transfer = true;
            gif_irq_on_done = false;
        }
        break;
    case 0x10:
        channels[2].madr = data & 0x01fffff0;
        printf("Writing 0x%08x to GIF.MADR\n", data & 0x01fffff0);
        break;
    case 0x20:
        channels[2].qwc = data;
        printf("Writing 0x%08x to GIF.QWC\n", data);
        break;
    case 0x30:
        channels[2].tadr = data;
        break;
    case 0x40:
        channels[2].asr[0] = data;
        break;
    case 0x50:
        channels[2].asr[1] = data;
        break;
    case 0x80:
        channels[2].sadr.data = data;
        break;
    }
}
void WriteIPUFROMChannel(uint32_t addr, uint32_t data)
{
    switch (addr & 0xff)
    {
    case 0x00:
        channels[3].chcr.data = data;
        if (channels[3].chcr.start)
            printf("[emu/DMAC]: Starting IPU_FROM transfer\n");
        break;
    case 0x10:
        channels[3].madr = data & 0x01fffff0;
        break;
    case 0x30:
        channels[3].tadr = data;
        break;
    case 0x40:
        channels[3].asr[0] = data;
        break;
    case 0x50:
        channels[3].asr[1] = data;
        break;
    case 0x80:
        channels[3].sadr.data = data;
        break;
    }
}
void WriteIPUTOChannel(uint32_t addr, uint32_t data)
{
    switch (addr & 0xff)
    {
    case 0x00:
        channels[4].chcr.data = data;
        if (channels[4].chcr.start)
            printf("[emu/DMAC]: Starting IPU_TO transfer\n");
        break;
    case 0x10:
        channels[4].madr = data & 0x01fffff0;
        break;
    case 0x30:
        channels[4].tadr = data;
        break;
    case 0x40:
        channels[4].asr[0] = data;
        break;
    case 0x50:
        channels[4].asr[1] = data;
        break;
    case 0x80:
        channels[4].sadr.data = data;
        break;
    }
}

bool sif0_ongoing_transfer = false, fetching_sif0_tag = true;
bool irq_on_done = false;
DMATag sif0_tag;

size_t buf_pos = 0;

struct SifCmdHeader
{
    uint32_t psize:8;
    uint32_t dsize:24;
    uint32_t dest;
    uint32_t cid;
    uint32_t opt;
};

struct SifInitPkt
{
    SifCmdHeader hdr;
    uint32_t buf;
};

struct SifCmdSRegData
{
    SifCmdHeader header;
    int index;
    uint32_t value;
};

struct SifRpcBindPkt
{
    SifCmdHeader header;
    int rec_id;
    uint32_t pkt_addr;
    int rpc_id;
    uint32_t client;
    uint32_t sid;
};

struct SifRpcCallPkt
{
    SifCmdHeader header;
    int rec_id;
    uint32_t pkt_addr;
    int rpc_id;
    uint32_t client;
    int rpc_number;
    int send_size;
    uint32_t receive;
    int recv_size;
    int rmode;
    uint32_t server;
};

uint32_t currentSvrId = 0;

const char* GetFuncName(uint32_t func_id)
{
    switch (currentSvrId)
    {
    case 0x80000006:
    {
        switch (func_id)
        {
        case 1:
            return "SifLoadElf";
        default:
            printf("Unknown LOADFILE func %d\n", func_id);
            exit(1);
        }
    }
    default:
        printf("Unknown server 0x%08x\n", currentSvrId);
        exit(1);
    }
}

void HandleSifCommand(SifCmdHeader* hdr)
{
    if ((hdr->cid & 0xF0000000) != 0x80000000)
        return;
    
    switch (hdr->cid)
    {
    case 0x80000001:
    {
        SifCmdSRegData* sregData = (SifCmdSRegData*)hdr;
        printf("sifSetSReg(0x%08x, 0x%08x)\n", sregData->index, sregData->value);
        break;
    }
    case 0x80000002:
    {
        SifInitPkt *initPkt = (SifInitPkt*)hdr;
        if (hdr->opt)
            printf("SIFCMD Init, opt=1 (finish initialization)\n");
        else
            printf("SIFCMD Init, buf=0x%08x\n", initPkt->buf);
        break;
    }
    case 0x80000009:
    {
        SifRpcBindPkt* pkt = (SifRpcBindPkt*)hdr;
        printf("SifRpcBind(0x%08x) (", pkt->sid);

        currentSvrId = pkt->sid;

        switch (pkt->sid)
        {
        case 0x80000001:
            printf("FILEIO");
            break;
        case 0x80000006:
            printf("LOADFILE");
            break;
        default:
            printf("Unknown server ID 0x%08x\n", pkt->sid);
            exit(1);
        }
        printf(")\n");
        break;
    }
    case 0x80000008:
        break;
    case 0x8000000A:
    {
        SifRpcCallPkt* pkt = (SifRpcCallPkt*)hdr;
        printf("SifRpcCall(%s)\n", GetFuncName(pkt->rpc_number));
        break;
    }
    default:
        printf("Unknown SIF command 0x%08x\n", hdr->cid);
        exit(1);
    }
}

void HandleSIF0Transfer()
{
	auto& c = channels[5];

	fetching_sif0_tag = true;

	if (!sif0_ongoing_transfer)
		return;
 
    if (c.qwc > 0)
	{
        while (c.qwc)
        {
            if (SIF::FIFO0_size() >= 4)
            {
                uint32_t data[4];
                for (int i = 0; i < 4; i++)
                    data[i] = SIF::ReadAndPopSIF0();
                
                __uint128_t qword = *(__uint128_t*)data;
                
                Bus::Write128(c.madr, {qword});

                c.qwc--;
                c.madr += 16;
            }
            else
                break;
        }
	}
	else if (irq_on_done)
	{
		printf("[emu/DMAC]: Transfer ended on SIF0 channel\n");

		stat.channel_irq |= (1 << 5);

		if (stat.channel_irq & stat.channel_irq_mask)
        {
            printf("[emu/DMAC]: Trigger SIF0 interrupt\n");
#ifdef EE_JIT
            EmotionEngine::SetIp1Pending();
#else
            EEInterpreter::SetIP1Pending();
#endif
        }

		c.chcr.start = 0;
        sif0_ongoing_transfer = false;
        irq_on_done = false;
	}
    else
    {
        if (SIF::FIFO0_size() >= 2)
        {
            uint32_t data[2];
            for (int i = 0; i < 2; i++)
                data[i] = SIF::ReadAndPopSIF0();
            
            sif0_tag.value = *(uint64_t*)data;
            printf("[emu/DMAC]: Read SIF0 tag 0x%08lx\n", (uint64_t)sif0_tag.value);

            c.qwc = sif0_tag.qwc;
            c.chcr.tag = (sif0_tag.value >> 16) & 0xffff;
            c.madr = sif0_tag.addr;
            c.tadr += 16;

            printf("[emu/DMAC]: Tag contains %d qwords, to be transferred to 0x%08x (%d)\n", c.qwc, c.madr, sif0_tag.irq);
            fetching_sif0_tag = false;

            if (c.chcr.tie && sif0_tag.irq)
                irq_on_done = true;
        }
    }

	if (sif0_ongoing_transfer)
	{
		Scheduler::Event sif0_evt;
		sif0_evt.cycles_from_now = 2;
		sif0_evt.func = HandleSIF0Transfer;
		sif0_evt.name = "SIF0 DMA transfer";

		Scheduler::ScheduleEvent(sif0_evt);
	}
}

void WriteSIF0Channel(uint32_t addr, uint32_t data)
{
	printf("[emu/DMAC]: Writing 0x%08x to %s of SIF0 channel\n", data, REG_NAMES[(addr >> 4) & 0xf]);

    switch (addr & 0xff)
    {
    case 0x00:
        channels[5].chcr.data = data;
        if (channels[5].chcr.start && (ctrl & 1))
		{
            printf("[emu/DMAC]: Starting SIF0 transfer\n");

			// We schedule the transfer 1 cycle from now
			// This is because the DMAC ticks at half the speed of the EE
			Scheduler::Event sif0_evt;
			sif0_evt.cycles_from_now = 1;
			sif0_evt.func = HandleSIF0Transfer;
			sif0_evt.name = "SIF0 DMA transfer";

			Scheduler::ScheduleEvent(sif0_evt);

			sif0_ongoing_transfer = true;
            fetching_sif0_tag = true;
            irq_on_done = false;
		}
        else
            sif0_ongoing_transfer = false;
        break;
    case 0x10:
        channels[5].madr = data & 0x01fffff0;
        break;
	case 0x20:
		channels[5].qwc = data & 0xffff;
		break;
    case 0x30:
        channels[5].tadr = data;
        break;
    case 0x40:
        channels[5].asr[0] = data;
        break;
    case 0x50:
        channels[5].asr[1] = data;
        break;
    case 0x80:
        channels[5].sadr.data = data;
        break;
    }
}

bool fetching_tag = true, ongoing_transfer = false;
bool sif1_irq_on_done = false;

DMATag tag;

void HandleSIF1Transfer()
{
	
	auto& c = channels[6];

	fetching_tag = true;

	if (!ongoing_transfer)
		return;
 
    if (c.qwc > 0)
	{
        while (c.qwc != 0)
        {
            uint128_t qword = Bus::Read128(c.madr);

            printf("[emu/DMAC]: Writing %s to SIF1 FIFO\n", print_128({qword.u128}));
            
            for (int i = 0; i < 4; i++)
                SIF::WriteFIFO1(qword.u32[i]);

            c.qwc--;
            c.madr += 16;
        }
	}
	else if (sif1_irq_on_done)
	{
		printf("[emu/DMAC]: Transfer ended on SIF1 channel\n");

		stat.channel_irq |= (1 << 6);

		if (stat.channel_irq & stat.channel_irq_mask)
        {
            printf("[emu/DMAC]: Trigger SIF1 interrupt\n");
#ifdef EE_JIT
            EmotionEngine::SetIp1Pending();
#else
#endif
        }

		c.chcr.start = 0;
        ongoing_transfer = false;
        sif1_irq_on_done = false;
	}
    else
    {
        auto address = c.tadr;

        tag.value = Bus::Read128(address).u128;

        printf("[emu/DMAC]: Read SIF1 tag %s from 0x%08x (%d qwords, from 0x%08x, tag_id %d)\n", print_128({tag.value}), address, tag.qwc, tag.addr, tag.tag_id);

        c.qwc = tag.qwc;
        c.chcr.tag = (tag.value >> 16) & 0xffff;

        uint16_t tag_id = tag.tag_id;
        switch (tag_id)
        {
        case 0:
            c.madr = tag.addr;
            c.tadr += 16;
            sif1_irq_on_done = true;
            break;
        case 1:
            c.madr = c.tadr+16;
            c.tadr = c.madr+(c.qwc*16);
            break;
        case 2:
            c.madr = c.tadr+16;
            c.tadr = tag.addr;
            break;
        case 3:
        case 4:
            c.madr = tag.addr;
            c.tadr += 16;
            break;
        default:
            printf("Unknown tag ID %d\n", tag.tag_id);
            exit(1);
        }

        if (c.chcr.tie && tag.irq)
            irq_on_done = true;
    }

	if (ongoing_transfer)
	{
		Scheduler::Event sif_evt;
		sif_evt.cycles_from_now = 2;
		sif_evt.func = HandleSIF1Transfer;
		sif_evt.name = "SIF1 DMA transfer";

		Scheduler::ScheduleEvent(sif_evt);
	}
}

void WriteSIF1Channel(uint32_t addr, uint32_t data)
{
	printf("[emu/DMAC]: Writing 0x%08x to %s of SIF1 channel\n", data, REG_NAMES[(addr >> 4) & 0xf]);

    switch (addr & 0xff)
    {
    case 0x00:
        channels[6].chcr.data = data;
		if (channels[6].chcr.start && (ctrl & 1))
		{
            printf("[emu/DMAC]: Starting SIF1 transfer\n");

			// We schedule the transfer 1 cycle from now
			// This is because the DMAC ticks at half the speed of the EE
			Scheduler::Event sif1_evt;
			sif1_evt.cycles_from_now = 1;
			sif1_evt.func = HandleSIF1Transfer;
			sif1_evt.name = "SIF1 DMA transfer";

			Scheduler::ScheduleEvent(sif1_evt);

			ongoing_transfer = true;
            fetching_tag = true;
            sif1_irq_on_done = false;
		}
        else
            ongoing_transfer = false;
        break;
    case 0x10:
        channels[6].madr = data & 0x01fffff0;
        break;
	case 0x20:
		channels[6].qwc = data;
		break;
    case 0x30:
        channels[6].tadr = data;
        break;
    case 0x40:
        channels[6].asr[0] = data;
        break;
    case 0x50:
        channels[6].asr[1] = data;
        break;
    case 0x80:
        channels[6].sadr.data = data;
        break;
    }
}

void WriteSIF2Channel(uint32_t addr, uint32_t data)
{
    switch (addr & 0xff)
    {
    case 0x00:
        channels[7].chcr.data = data;
        if (channels[7].chcr.start)
            printf("[emu/DMAC]: Starting SIF2 transfer\n");
        break;
    case 0x10:
        channels[7].madr = data & 0x01fffff0;
        break;
    case 0x30:
        channels[7].tadr = data;
        break;
    case 0x40:
        channels[7].asr[0] = data;
        break;
    case 0x50:
        channels[7].asr[1] = data;
        break;
    case 0x80:
        channels[7].sadr.data = data;
        break;
    }
}

void WriteSPRFROMChannel(uint32_t addr, uint32_t data)
{
    switch (addr & 0xff)
    {
    case 0x00:
        channels[8].chcr.data = data;
        if (channels[8].chcr.start)
            printf("[emu/DMAC]: Starting SPR_FROM transfer\n");
        break;
    case 0x10:
        channels[8].madr = data & 0x01fffff0;
        break;
    case 0x30:
        channels[8].tadr = data;
        break;
    case 0x40:
        channels[8].asr[0] = data;
        break;
    case 0x50:
        channels[8].asr[1] = data;
        break;
    case 0x80:
        channels[8].sadr.data = data;
        break;
    }
}
void WriteSPRTOChannel(uint32_t addr, uint32_t data)
{
    switch (addr & 0xff)
    {
    case 0x00:
        channels[9].chcr.data = data;
        if (channels[9].chcr.start)
            printf("[emu/DMAC]: Starting SPR_TO transfer\n");
        break;
    case 0x10:
        channels[9].madr = data & 0x01fffff0;
        break;
    case 0x30:
        channels[9].tadr = data;
        break;
    case 0x40:
        channels[9].asr[0] = data;
        break;
    case 0x50:
        channels[9].asr[1] = data;
        break;
    case 0x80:
        channels[9].sadr.data = data;
        break;
    }
}

uint32_t ReadGIFChannel(uint32_t addr)
{
    switch (addr & 0xff)
    {
    case 0x00:
        return channels[2].chcr.data;
    case 0x10:
        return channels[2].madr;
    case 0x30:
        return channels[2].tadr;
    case 0x40:
        return channels[2].asr[0];
    case 0x50:
        return channels[2].asr[1];
    case 0x80:
        return channels[2].sadr.data;
    }
}

uint32_t ReadSIF0Channel(uint32_t addr)
{
	switch (addr & 0xff)
    {
	case 0x00:
		return channels[5].chcr.data;
	case 0x20:
		return channels[5].qwc;
	}
}

uint32_t ReadSIF1Channel(uint32_t addr)
{
	switch (addr & 0xff)
    {
	case 0x00:
		return channels[6].chcr.data;
	case 0x20:
		return channels[6].qwc;
	case 0x30:
		return channels[6].tadr;
	}
}

uint32_t ReadDENABLE()
{
	return d_enable;
}

void WriteDENABLE(uint32_t data)
{
	d_enable = data;
}

uint32_t dpcr;
uint32_t sqwc;

void WriteDSTAT(uint32_t data)
{
	printf("[emu/DMAC]: Writing 0x%08x to D_STAT\n", data);

    stat.clear &= ~(data & 0xffff);
    stat.reverse ^= (data >> 16);

#ifdef EE_JIT
	if (stat.channel_irq & stat.channel_irq_mask)

        EmotionEngine::SetIp1Pending();
    else
        EmotionEngine::ClearIp1Pending();
#else
    if (stat.channel_irq & stat.channel_irq_mask)
        EEInterpreter::SetIP1Pending();
    else
        EEInterpreter::ClearIP1Pending();
#endif
}

uint32_t ReadDSTAT()
{
    printf("[emu/DMAC]: Reading 0x%08x from D_STAT\n", stat.value);
    return stat.value;
}

void WriteDCTRL(uint32_t data)
{
    ctrl = data;
}

uint32_t ReadDCTRL()
{
	return ctrl;
}

void WriteDPCR(uint32_t data)
{
    dpcr = data;
}

uint32_t ReadDPCR()
{
    return dpcr;
}

void WriteSQWC(uint32_t data)
{
    sqwc = data;
}

bool GetCPCOND0()
{
    return (~(dpcr & 0x3FF) | stat.channel_irq) == 0x3FF;
}

}  // namespace DMAC
