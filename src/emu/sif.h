#pragma once

#include <util/uint128.h>
#include <queue>

struct SIFRegs
{
    uint32_t mscom;
    uint32_t smcom;
    uint32_t msflag;
    uint32_t smflag;
    uint32_t ctrl;
    uint32_t padding;
    uint32_t bd6;
};

class SubsystemInterface
{
private:
    SIFRegs regs = {};
public:
    std::queue<uint32_t> fifo0, fifo1;

    void write(uint32_t addr, uint32_t data);
    uint32_t read(uint32_t addr);
};
