// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include <emu/dev/sif.h>

#include <queue>

uint32_t sif_ctrl;
uint32_t bd6;
uint32_t mscom;
uint32_t smcom;
uint32_t msflg;
uint32_t smflg;

std::queue<uint32_t> fifo0, fifo1;

void SIF::WriteMSCOM_EE(uint32_t data)
{
	mscom = data;
}

void SIF::WriteMSFLG_EE(uint32_t data)
{
	msflg |= data;
}

void SIF::WriteSMFLG_EE(uint32_t data)
{
	smflg &= ~data;
}

void SIF::WriteCTRL_EE(uint32_t data)
{
	if (!(data & 0x100))
		sif_ctrl &= ~0x100;
	else
		sif_ctrl |= 0x100;
}

void SIF::WriteBD6_EE(uint32_t data)
{
	bd6 = data;
}

void SIF::WriteCTRL_IOP(uint32_t data)
{
	uint8_t bark = data & 0xF0;

    if (data & 0xA0)
    {
        sif_ctrl &= ~0xF000;
        sif_ctrl |= 0x2000;
    }

    if (sif_ctrl & bark)
        sif_ctrl &= ~bark;
    else
        sif_ctrl |= bark;
}

void SIF::WriteSMCOM_IOP(uint32_t data)
{
	smcom = data;
}

void SIF::WriteSMFLG_IOP(uint32_t data)
{
	smflg = data;
}

void SIF::WriteMSFLG_IOP(uint32_t data)
{
	msflg &= ~data;
}

uint32_t SIF::ReadMSCOM_EE()
{
	return mscom;
}

uint32_t SIF::ReadSMCOM()
{
	return smcom;
}

uint32_t SIF::ReadMSFLG()
{
	return msflg;
}

uint32_t SIF::ReadSMFLG()
{
	return smflg;
}

uint32_t SIF::ReadCTRL()
{
	return sif_ctrl;
}

size_t SIF::FIFO0_size()
{
	return fifo0.size();
}

size_t SIF::FIFO1_size()
{
	return fifo1.size();
}

void SIF::WriteFIFO0(uint32_t data)
{
	fifo0.push(data);
}

void SIF::WriteFIFO1(uint32_t data)
{
	fifo1.push(data);
}

uint32_t SIF::ReadAndPopSIF1()
{
	uint32_t data = fifo1.front();
	fifo1.pop();

	return data;
}
