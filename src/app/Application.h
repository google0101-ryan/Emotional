#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>

class Application
{
private:
    static bool isRunning;
    static int exit_code;
public:
    static bool Init(int argc, char** argv);
    static int Run();
    static void Exit(int code);
    static void Exit();
    static void Dump();
};