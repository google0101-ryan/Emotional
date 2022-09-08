#pragma once

#include <cstddef> // size_t
#include <emu/Bus.h> // class Bus
#include <fstream> // ifstrem
#include <util/uint128.h>

Bus::Bus(std::string fileName, bool& s)
{
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);

    if (!file.is_open())
    {
        printf("[emu/Bus]: %s: Couldn't open file %s. Is it available?\n", __FUNCTION__, fileName.c_str());
        s = false;
        return;
    }

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size < 0x400000)
    {
        printf("[emu/Bus]: %s: Expected file of size 0x400000, got file of size 0x%06lx\n", __FUNCTION__, size);
        s = false;
        return;
    }

    file.read((char*)bios, size);

    s = true;
    printf("[emu/Bus]: %s: Bus initialized\n", __FUNCTION__);

    console.open("log.txt");
}