#include "cdvd.h"

uint8_t N_status = (1 << 6);

uint8_t CDVD::ReadNStatus()
{
	return N_status;
}