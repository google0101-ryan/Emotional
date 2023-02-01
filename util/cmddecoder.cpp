#include "../src/util/uint128.h"

union SifRpcHeader
{
	__uint128_t data;
	struct
	{
		__uint128_t psize : 8;
		__uint128_t dsize : 24;
		__uint128_t dest : 32;
		__uint128_t cid : 32;
		__uint128_t opt : 32;
	};
};

uint8_t data[] = {0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00};

int main()
{
	SifRpcHeader hdr;
	hdr.data = *(__uint128_t*)data;

	printf("CMD ID: 0x%08x, %d\n", hdr.cid, hdr.opt);
}