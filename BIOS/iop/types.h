#pragma once

typedef struct
{
	unsigned int ram_size;
	unsigned int boot_info;
	unsigned int cmd_ptr;
	unsigned int unk2[3];
	unsigned int numModules;
	unsigned int structEnd;
} IopBootInfo;