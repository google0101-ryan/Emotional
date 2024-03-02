#include <stddef.h>
#include <loadcore/global.h>
#include <sifman/global.h>

ExportTable_t table =
{
	.magic = 0x41C00000,
	.next = NULL,
	.version = 0x101,
	.mode = 0,
	.name = "sifman",
	.exports = {
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		sifCmdInit,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		ExportStub,
		sceSifCheckInit,
	}
};

int main(int argc, char** argv)
{
	RegisterLibraryEntries(&table);
	return 0;
}