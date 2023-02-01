#pragma once

#include <cstdint>
#include <cstdio>

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

uint8_t data[] = {0x03, 0x00, 0x00, 0x30, 0x40, 0xe3, 0x01, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

int main()
{
	DMATag tag;

	tag.value = *(__uint128_t*)data;

	printf("QWC: 0x%04x, prio_control: %d, tag_id: %d, irq: %d, addr: 0x%08x, mem_sel: %d, data_transfer: 0x%08lx\n", tag.qwc, tag.prio_control, tag.tag_id, tag.irq, tag.addr, tag.mem_sel, tag.data_transfer);

	printf("%d\n", 0x1f % 0x1f);

	return 0;
}