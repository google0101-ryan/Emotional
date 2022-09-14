#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>

class EmotionTimers
{
private:
    struct Timer
    {
        uint32_t count;
        uint32_t mode;
        uint32_t comp;
        uint32_t hold;
    } timers[4];
public:
    void write(uint32_t addr, uint32_t data);
};