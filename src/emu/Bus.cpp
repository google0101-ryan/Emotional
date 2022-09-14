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

    gif = new GIF();
    gs = new GraphicsSynthesizer();
    ee_timers = new EmotionTimers();
    ee_dmac = new EmotionDma();
    vu0 = new VectorUnit(0);
    vu1 = new VectorUnit(1);

    vif0 = new VectorInterface(vu0, 0);
    vif1 = new VectorInterface(vu1, 1);

    sif = new SubsystemInterface();
}

void Bus::Clock()
{
    gif->tick(16);
}