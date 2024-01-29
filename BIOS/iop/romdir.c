#include "romdir.h"

RomDir *SearchForRomDir(void *start, void *end, RomDir *out)
{
	if (start >= end)
		return NULL;
	
	unsigned int* offsPtr = (unsigned int*)((char*)start + 0x1c); // Offset from 'RESET' string to ROMDIR offset
	unsigned int* magicPtr = (unsigned int*)(start);

	do
	{
		if (*magicPtr == 0x45534552)
		{
			out->searchBegin = start;
			out->firstEntry = magicPtr;
			out->romDirSize = *offsPtr;
			return out;
		}

		offsPtr++;
		magicPtr++;
	} while (magicPtr < end);

	out->searchBegin = NULL;
	return NULL;
}

/**
 * @brief The actual structure found within the ROM
*/
typedef struct
{
	char name[10];
	unsigned short ext_info_size;
	unsigned int file_size;
} RomEnt;

/** 
 * @brief This finds a ROMDIR entry with the name `name`
 * @param dir Pointer to the ROM directory info structure
 * @param name The name of the entry (E.X. IOPBTCONF)
 * @param out Output info on the ROM entry
*/
RomDirEnt *FindRomDirEnt(RomDir *dir, char *name, RomDirEnt *out)
{
	RomEnt* entry = (RomEnt*)dir->firstEntry;
	char fname[12];
	for (int i = 0; i < 12; i++)
	{
		if (*name < '!') break;
		fname[i] = *name;
		name++;
	}

	unsigned int fileOffs = 0;
	unsigned int infoOffs = 0;
	while (entry->name[0])
	{
		if (*(unsigned int*)&fname[0] == *(unsigned int*)&entry->name[0]
			&& *(unsigned int*)&fname[4] == *(unsigned int*)&entry->name[4]
			&& fname[8] == entry->name[8]
			&& fname[9] == entry->name[9])
		{
			out->name = entry->name;
			out->extInfo = NULL;
			out->filePtr = (void*)((char*)dir->searchBegin + fileOffs);
			if (entry->ext_info_size)
			{
				out->extInfo = (void*)((char*)dir->searchBegin + infoOffs);
			}

			return out;
		}
		
		fileOffs += entry->file_size;
		infoOffs += entry->ext_info_size;
		entry++;
	}

	return NULL;
}
