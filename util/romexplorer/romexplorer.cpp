#include <cstdint>
#include <fstream>
#include <cstring>
#include <vector>

struct romdir_entry
{
    char name[10];
    uint16_t ext_info_size;
    uint32_t file_size;
};

struct Entry
{
    romdir_entry* entry;
    uint32_t pos;
    uint32_t end;
};

std::vector<Entry> entries;

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("Usage: %s <bios>\n", argv[0]);
        return 0;
    }

    std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    uint8_t* bios = new uint8_t[size];
    file.read((char*)bios, size);

    int resetdir_pos = -1;

    for (int i = 0; i < size; i++)
    {
        if (!strncmp((const char*)bios+i, "RESET", 5))
        {
            resetdir_pos = i;
            break;
        }
    }

    if (resetdir_pos == -1)
    {
        printf("Could not find RESET entry in BIOS\n");
        return -1;
    }

    printf("Found romdir at 0x%08x\n", 0xbfc00000 + resetdir_pos);

    uint32_t start_pos = 0;

    for (auto i = (romdir_entry*)&bios[resetdir_pos]; i->name[0] != '\0'; i++)
    {
        Entry entry;
        entry.entry = i;
        entry.pos = start_pos;
        entry.end = start_pos + i->file_size;
        start_pos += i->file_size;

        entries.push_back(entry);

        printf("Found romdir entry %s (from 0x%08x to 0x%08x)\n", i->name, 0xbfc00000 + entry.pos, 0xbfc00000 + entry.end);
    }

    printf("0x%08x\n", 0xbfc00000 + start_pos);
}