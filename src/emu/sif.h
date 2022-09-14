#pragma once

#include <util/uint128.h>

class SubsystemInterface
{
private:
    uint32_t sif_ctrl;
    uint32_t sif_bd6;
    uint32_t mscom, smcom; // Master-to-slave communication (EE -> IOP)
    uint32_t msflg, smflg; // Master-to-slave semaphore, and vice versa
public:
    void write_ee(uint32_t addr, uint32_t data);
    uint32_t read_ee(uint32_t addr);
};