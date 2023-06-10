// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include <emu/dev/cdvd.h>
#include "cdvd.h"
#include <queue>
#include <cstdio>
#include <cstdlib>
#include <ctime>

uint8_t N_status = (1 << 6);

std::queue<uint8_t> results;

uint8_t CDVD::ReadNStatus()
{
    return N_status;
}

uint8_t CDVD::ReadSStatus()
{
    return results.empty() << 6;
}

static const uint8_t monthmap[13] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
#define btoi(b) ((b)/16*10 + (b)%16)    /* BCD to u_char */
#define itob(i) ((i)/10*16 + (i)%10) 

uint8_t currentcmd = 0;

void CDVD::AddSCommand(uint8_t cmd)
{
	currentcmd = cmd;
	switch (cmd)
	{
	case 0x08:
	{
		time_t t = time(NULL);
		tm* tim = localtime(&t);

		results.push(0);
		results.push(itob(tim->tm_sec));
		results.push(itob(tim->tm_min));
		results.push(itob(tim->tm_hour));
		results.push(0);
		results.push(itob(tim->tm_mday));
		results.push(itob(tim->tm_mon));
		results.push(itob(tim->tm_year));
		break;
	}
	default:
		printf("[emu/CDVD]: Unknown S-command 0x%02x\n", cmd);
		exit(1);
	}
}

uint8_t CDVD::ReadSCommand()
{
    return currentcmd;
}

uint8_t CDVD::ReadSResult()
{
	uint8_t data = results.front();
	results.pop();
	return data;
}
