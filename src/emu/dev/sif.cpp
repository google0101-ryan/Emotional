#include "sif.h"

uint32_t sif_ctrl;
uint32_t bd6;
uint32_t mscom;
uint32_t smcom;
uint32_t msflg;
uint32_t smflg;

void SIF::WriteMSCOM_EE(uint32_t data)
{
	mscom = data;
}

void SIF::WriteMSFLG_EE(uint32_t data)
{
	msflg |= data;
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
	return 0;
}
