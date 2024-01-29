#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <cstring>

struct RomdirEntry
{
    char name[10];
    uint16_t ext_info_size;
    uint32_t file_size;
} __attribute__((packed));

static_assert(sizeof(RomdirEntry) == 16);

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        printf("Usage: %s <bios file> <rom file>\n", argv[0]);
        printf("\tExtracts a file from a PS2 BIOS image\n");
        return 0;
    }

    std::ifstream file(argv[1], std::ios::ate | std::ios::binary);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    uint8_t* buf = new uint8_t[size];
    
    file.read((char*)buf, size);

    uint32_t romdir_start = 0;

    // Search for the first instance of "RESET"
    for (size_t i = 0; i < size; i++)
    {
        if (!std::strncmp("RESET", (const char*)&buf[i], 5))
        {
            printf("Found \"RESET\" signature at 0x%08x\n", (uint32_t)i);
            romdir_start = (uint32_t)i;
            break;
        }
    }

    if (romdir_start == 0)
    {
        printf("ERROR: Invalid PS2 BIOS, no romdir found\n");
        return -1;
    }

    RomdirEntry* entries = (RomdirEntry*)(buf+romdir_start);

    int entryCount = 0;
    while (entries[entryCount++].name[0] != 0);

    printf("%d entries in ROMDIR\n", entryCount);

    uint32_t pos = 0;
    for (int i = 0; i < entryCount; i++)
    {
        printf("Found entry %s, at 0x%08x\n", entries[i].name, pos);
        pos += entries[i].file_size;
    }

    pos = 0;

    for (int i = 0; i < entryCount; i++)
    {
        printf("%s\t->\t0x%08x, 0x%08x\n", entries[i].name, pos, entries[i].file_size);
        if (!strncmp(entries[i].name, argv[2], 10))
        {
            printf("Found \"%s\" at 0x%08x\n", entries[i].name, pos);

            std::ofstream out(argv[2]);

            out.write((char*)(buf+pos), entries[i].file_size);
            out.close();

            break;
        }
        else
            pos += entries[i].file_size;
    }

    return 0;
}