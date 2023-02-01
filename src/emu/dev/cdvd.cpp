// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#include <emu/dev/cdvd.h>

uint8_t N_status = (1 << 6);

uint8_t CDVD::ReadNStatus()
{
	return N_status;
}
