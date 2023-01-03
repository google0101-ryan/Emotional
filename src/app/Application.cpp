#include "Application.h"
#include <signal.h>
#include <emu/System.h>

bool Application::isRunning = false;
int Application::exit_code = 0;

void Sig(int)
{
    printf("Segfault\n");
    Application::Exit();
}

bool Application::Init(int argc, char** argv)
{
	if (argc < 2)
    {
        printf("Usage: %s [bios]\n", argv[0]);
        return false;
    }

    bool success = false;

    printf("[app/App]: %s: Initializing System\n", __FUNCTION__);

	System::Reset();
	System::LoadBios(argv[1]);

    std::atexit(Application::Exit);
    signal(SIGSEGV, Sig);
    signal(SIGINT, Application::Exit);
    
    isRunning = true;

    return true;
}

int Application::Run()
{
	System::Run();
}

void Application::Exit(int code)
{
    exit_code = code;
    isRunning = false;
    exit(1);
}

void Application::Exit()
{
	System::Dump();
}