#pragma once

typedef struct ExportTable_t
{
	unsigned int magic;
	struct ExportTable_t* next;
	unsigned short version;
	unsigned short mode;
	char name[8];
	void* exports[];
} ExportTable_t;

void RegisterLibraryEntries(ExportTable_t* table);

static void ExportStub()
{
	// Do nothing export stub for placeholder slots
}

int* QueryBootMode(int mode);