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
    ee->Dump();
    isRunning = false;
}