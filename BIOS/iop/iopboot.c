#include "types.h"
#include "string.h"
#include "romdir.h"
#include "btconf.h"

IopBootInfo* gBootInfo = (IopBootInfo*)0x20000;

/**
 * @brief The entrypoint of the IOPBOOT module, which bootstraps the IOP kernel
 * @param ram_size The size of the IOP's RAM, in megabytes.
 * @param boot_info This gets used to load IOPBTCONFx, where x is boot_info's value
 * @param udnl_cmd The command line, usually NULL on retail PS2s
*/
void _entry(int ram_size, int boot_info, char* udnl_cmd, int unk)
{
	// Disable interrupts
	asm volatile("mtc0 $0,$12");

	// The IOP kernel can only access a maximum of 2MBs of RAM
	if (ram_size > 2)
		ram_size = 2;
	
	// In the real IOPBOOT, the stack pointer is set to the end of RAM here

	// This structure is used to pass around some boot info
	gBootInfo->cmd_ptr = 0;
	gBootInfo->structEnd = 0x20020;
	gBootInfo->ram_size = ram_size;
	gBootInfo->boot_info = boot_info;
	if (udnl_cmd)
	{
		// Copy the command line to just past the structure
		gBootInfo->cmd_ptr = ((char*)gBootInfo+sizeof(IopBootInfo));
		strcpy(((char*)gBootInfo+sizeof(IopBootInfo)), udnl_cmd);
		size_t cmdlen = strlen(udnl_cmd);
		// 8 byte align the struct's end
		gBootInfo->structEnd += ((cmdlen + 8) & 0xfffffffc);
	}

	RomDir romDir;
	if (!SearchForRomDir((void*)0xbfc00000, (void*)0xbfc10000, &romDir))
	{
		// If we can't find any ROMDIR entries, just crash
		do {} while(1);
	}

	// One of IOPBTCONF entries
	char configName[48];
	strcpy(configName, "IOPBTCONF");
	configName[9] = boot_info + '0'; // This means we load IOPBTCONFx, where x is boot_info
	RomDirEnt btconf;
	if (!FindRomDirEnt(&romDir, configName, &btconf))
	{
		// If IOPBTCONFx doesn't exist, load IOPBTCONF
		configName[9] = 0;
		strcpy(configName, "IOPBTCONF");
		if (!FindRomDirEnt(&romDir, configName, &btconf))
			do {} while (1);
	}

	int moduleCount = 0;

	// Now we parse this list, which tells us which modules to load
	// The name ptr is 10 bytes, followed by 2 bytes for ext_info_size
	// At 0xC is the file size, so we can dereference that and add it to the file ptr to get the end
	char* end = btconf.filePtr + *(unsigned int*)((char*)btconf.name + 0xc);
	char* buf = (char*)btconf.filePtr;
	while (buf < end)
	{
		// skip whitespace
		while (*buf < ' ') buf++;
		// New entry, non-whitespace
		moduleCount++;
	}
	gBootInfo->numModules = moduleCount;
	int moduleLoadAddr = 0;

	buf = (char*)btconf.filePtr;

	// Following the IopBootInfo struct is a list of addresses to be loaded
	unsigned int* endOfStructPtr = (unsigned int*)gBootInfo->structEnd;
	RomDirEnt toLoad;

	while (buf < end)
	{
		char c = *buf;
		// '@' characters are used to specify the module load offset
		if (c == '@')
		{
			buf++;
			moduleLoadAddr = ParseLoadAddr(&buf);
		}
		// Either used to include other BTCONF files
		// Or used to call a function somewhere in memory
		else if (c == '!')
		{
			// BUG: The PS2 BIOS doesn't support !includes, at least on scph10000
			if (strncmp(buf, "!addr", 6) == 0)
			{
				buf += 6;
				*endOfStructPtr = ParseLoadAddr(&buf) * 4 + 1;
				endOfStructPtr++;
				*endOfStructPtr = 0;
			}
		}
		else if (c != '#')
		{
			// Find the module to be loaded
			void* ret = FindRomDirEnt(&romDir, buf, &toLoad);
			if (!ret)
				return 1;
			*endOfStructPtr = toLoad.filePtr;
			endOfStructPtr++;
			*endOfStructPtr = 0;
		}
		if (buf > end) break;
		// Skip trailing whitespace
		do
		{
			if (*buf < ' ') break;
			buf++;
		} while (buf < end);
		do
		{
			if (0x1f < *buf) break;
			buf++;
		} while (buf < end);
	}
	// Now we've got a list of addresses following IopBootInfo, terminated with a 0 word
	// Load them, in order
}