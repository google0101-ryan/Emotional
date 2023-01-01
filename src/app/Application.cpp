#include "Application.h"
#include "emu/cpu/EmotionEngine.h"
#include <signal.h>

bool Application::isRunning = false;
int Application::exit_code = 0;

Bus* Application::bus = nullptr;
EmotionEngine* Application::ee = nullptr;

void Sig(int)
{
    printf("Segfault\n");
    Application::Exit();
}

SDL_Window* window;
SDL_GLContext context;

bool Application::Init(int argc, char** argv)
{
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	window = SDL_CreateWindow("Yay, Playstation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	context = SDL_GL_CreateContext(window);

	if (gl3wInit())
	{
		printf("GL3W initialization error\n");
		exit(1);
	}

	glEnable(GL_DEPTH_TEST);

    if (argc < 3)
    {
        printf("Usage: %s [bios] [elf] --fast-boot\n", argv[0]);
        return false;
    }

    bool success = false;

    printf("[app/App]: %s: Initializing Bus\n", __FUNCTION__);

    bus = new Bus(argv[1], argv[2], success);

	if (argc >= 4)
	{
		int current = 3;
		argc -= 3;
		while (argc)
		{
			if (!strncmp(argv[current], "--fast-boot", 11))
			{
				bus->FastBoot();
			}
			else if (!strncmp(argv[current], "--dont-clip", 11))
			{
				shouldCullFb = false;
			}
			current++;
			argc--;
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

bool shouldCullFb = true;

int Application::Run()
{
    uint8_t* out = new uint8_t[2048 * 2048 * 4];
    while (isRunning)
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
				bus->GetBus()->TriggerInterrupt(0);

				bus->GetGS()->vram.bind();
				glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);

				SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(out, 2048, 2048, 8*4, 2048 * 4, 0xFF, 0xFF00, 0xFF0000, 0xFF000000);
				SDL_Surface* window_surface = SDL_GetWindowSurface(window);

				uint64_t dispfb = (bus->GetGS()->priv_regs.pmode & 1) ? bus->GetGS()->priv_regs.dispfb1 : bus->GetGS()->priv_regs.dispfb2;
				uint64_t display = (bus->GetGS()->priv_regs.pmode & 1) ? bus->GetGS()->priv_regs.display1 : bus->GetGS()->priv_regs.display2;

				uint64_t fb_base = (dispfb & 0x1FF) * 2048;
				uint64_t fb_width = (display >> 32) & 0x7FF;
				uint64_t fb_height = (display >> 44) & 0x7FF;
				fb_width += 1;
				fb_height += 1;

				// printf("Framebuffer is at %d, %d (%dx%d), surface is %dx%d\n", fb_base % 2048, fb_base / 2048, fb_width, fb_height, surface->w, surface->h);

				SDL_Rect rect;
				rect.x = fb_base % 2048;
				rect.y = fb_base / 2048;
				rect.w = 640;
				rect.h = 224;
				
				if (shouldCullFb)
					SDL_BlitSurface(surface, &rect, window_surface, NULL);
				else
					SDL_BlitSurface(surface, NULL, window_surface, NULL);
				SDL_UpdateWindowSurface(window);
			}
		}

		bus->GetBus()->TriggerInterrupt(11);
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

	if (window)
		SDL_DestroyWindow(window);
}