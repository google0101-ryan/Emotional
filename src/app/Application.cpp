#include "Application.h"
#include "emu/cpu/EmotionEngine.h"
#include <signal.h>

bool Application::isRunning = false;
int Application::exit_code = 0;

Bus* Application::bus = nullptr;
EmotionEngine* Application::ee = nullptr;
Window* Application::window = nullptr;

void Sig(int)
{
    printf("Segfault\n");
    Application::Exit();
}

bool Application::Init(int argc, char** argv)
{
    if (argc < 3)
    {
        printf("Usage: %s [bios] [elf] --fast-boot\n", argv[0]);
        return false;
    }

	window = new Window();
	Renderer* renderer = new Renderer();
	window->AttachRenderer(renderer);

    bool success = false;

    printf("[app/App]: %s: Initializing Bus\n", __FUNCTION__);

    bus = new Bus(argv[1], argv[2], success, renderer);

	if (argc >= 4)
	{
		if (!strncmp(argv[3], "--fast-boot", 11))
		{
			bus->FastBoot();
		}
	}

    if (!success)
    {
        printf("[app/App]: %s: Error initializing Bus\n", __FUNCTION__);
        return false;
    }

    ee = new EmotionEngine(bus, bus->GetVU0());

    std::atexit(Application::Exit);
    signal(SIGSEGV, Sig);
    signal(SIGINT, Application::Exit);
    
    isRunning = true;

    return true;
}

static uint64_t total_cycles = 0;
bool vblank_started = false;

int Application::Run()
{
    while (isRunning && !window->ShouldClose())
    {
		while (total_cycles < 4919808)
		{
            ee->Clock(32);
            bus->Clock();

            total_cycles += 32;

			if (!vblank_started && total_cycles >= 4498432)
			{
				vblank_started = true;
				bus->GetGS()->GetCSR().vsint = true;

				bus->GetGS()->GetCSR().field = !bus->GetGS()->GetCSR().field;

				if (!(bus->GetGS()->GetIMR() & 0x800))
					bus->trigger(0);
				
				bus->trigger(2);
				//bus->GetBus()->TriggerInterrupt(0);
			}


		}

		//bus->GetBus()->TriggerInterrupt(11);
		bus->trigger(3);
		vblank_started = false;
		total_cycles = 0;
		bus->GetGS()->GetCSR().vsint = false;
    }
    return exit_code;
}

void Application::Exit(int code)
{
    exit_code = code;
    isRunning = false;
    exit(1);
}

void Application::Exit()
{
    ee->Dump();
    // bus->GetVU0()->Dump();
    bus->GetIop()->Dump();
    bus->Dump();

    std::ofstream mem("mem.dump");

    for (int i = 0x70000000; i < 0x70004000; i++)
        mem << bus->read<uint8_t>(i);
    mem.flush();
    mem.close();

    mem.open("ram.dump");

    for (int i = 0; i < 0x2000000; i++)
        mem << bus->read<uint8_t>(i);
    mem.flush();
    mem.close();
}