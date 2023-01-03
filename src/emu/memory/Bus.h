#pragma once

#include <cstdint>

namespace Bus
{

void LoadBios(uint8_t* data);

uint32_t Read32(uint32_t addr);

}