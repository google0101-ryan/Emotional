#include "Application.h"
#include "emu/cpu/EmotionEngine.h"

bool Application::isRunning = false;
int Application::exit_code = 0;

Bus* Application::bus = nullptr;
EmotionEngine* Application::ee = nullptr;

bool Application::Init(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("Usage: %s [bios]\n", argv[0]);
        return false;
    }

    bool success = false;

    printf("[app/App]: %s: Initializing Bus\n", __FUNCTION__);

    bus = new Bus(argv[1], success);

    if (!success)
    {
        printf("[app/App]: %s: Error initializing Bus\n", __FUNCTION__);
        return false;
    }

    ee = new EmotionEngine(bus);

    std::atexit(Application::Exit);

    isRunning = true;

    return true;
}

int Application::Run()
{
    while (isRunning)
    {
        ee->Clock();
    }
    return exit_code;
}

void Application::Exit(int code)
{
    exit_code = code;
    isRunning = false;
}

void Application::Exit()
{
    ee->Dump();

    std::ofstream mem("mem.dump");

    for (int i = 0x70000000; i < 0x70004000; i++)
        mem << bus->read<uint8_t>(i);
    mem.flush();
    mem.close();
}