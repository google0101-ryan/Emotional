#pragma once

#include <cstdio>
#include <string>
#include <fstream>

namespace System
{

void LoadBios(std::string biosName);

void Reset();
void Run();
void Dump();

}