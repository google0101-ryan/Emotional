#include "Bus.h"
#include <cstring>

uint8_t BiosRom[0x400000];

void Bus::LoadBios(uint8_t *data)
{
	memcpy(BiosRom, data, 0x400000);
}