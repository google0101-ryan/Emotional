#include "threading.h"
#include <stdint.h>

typedef struct LinkedList
{
	LinkedList* next;
	LinkedList* last;
} LinkedList;

struct unknown0
{
	uint32_t unk32;
	char unk[72];
} unknown0[256];

LinkedList lists[128];

int DAT_800125ec, DAT_800125f0, DAT_80016fe0;

void InitEENULL()
{
	for (int i = 0x80; i >= 0; i--)
	{
		lists[i].next = &lists[i];
		lists[i].last = &lists[i];
	}

	DAT_80016fe0 = 0;
	DAT_800125ec = 0;
	DAT_800125f0 = 0;

	for (int i = 0; i < 256; i++)
	{
		unknown0[i].unk32 = 0;
		
	}
}