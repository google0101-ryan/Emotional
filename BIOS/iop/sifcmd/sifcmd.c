#include <loadcore/global.h>
#include <sifman/global.h>
#include <intrman/global.h>
#include <stdlib.h>
#include <stdint.h>

typedef volatile uint32_t vu32;

#define SIF_SMCOM (*(vu32*)0xbd000010)

int IrqHandler(void* arg)
{

}

ExportTable_t exportTable =
{
	.magic = 0x41C00000,
	.next = NULL,
	.version = 0x101,
	.mode = 0,
	.name = "sifman",
	.exports =
	{

	}
};

char RecvBuf[80];

int main(int argc, char** argv)
{
	int* bootMode = QueryBootMode(3);
	if (bootMode == NULL)
	{
		int sifCmdInited = sceSifCheckInit();
		if (!sifCmdInited)
			sifCmdInit();
		
		RegisterLibraryEntries(&exportTable);

		if (!sifCmdInited)
		{
			RegisterIntrHandler(0x2b, 1, &IrqHandler, NULL);
			EnableIntr(0x22b);
			SIF_SMCOM = &RecvBuf;
		}
	}

	return 0;
}