#pragma once

#include <stddef.h>

typedef struct
{
	void* searchBegin; // The beginning of the search range (TODO: why does the BIOS save this?)
	void* firstEntry;
	size_t romDirSize;
} RomDir;

typedef struct
{
	char* name;
	void* filePtr;
	void* extInfo; 
} RomDirEnt;

RomDir* SearchForRomDir(void* start, void* end, RomDir* out);
RomDirEnt* FindRomDirEnt(RomDir* dir, char* name, RomDirEnt* out);