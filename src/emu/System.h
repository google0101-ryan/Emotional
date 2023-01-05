// (c) Copyright 2022 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)


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