#include <app/Application.h>
#include <cstdlib>
#include <cstdio>

int main(int argc, char** argv)
{
    if (!Application::Init(argc, argv))
    {
        printf("[src/Main]: %s: Error initializing main app class\n", __FUNCTION__);
        exit(1);
    }

    return Application::Run();
}