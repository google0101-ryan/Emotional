#pragma once

#include <emu/cpu/ee/vu.h>
#include <queue>

// Used to upload compressed data to a VectorUnit via DMA
class VectorInterface
{
private:
    VectorUnit* vu;
    int id;
    uint32_t fbrst; // Used to reset various states in the VIF
    uint32_t stat; // Various status bits
    uint32_t err; // Controls error and interrupt behavior
    uint32_t fifo_size; // Varies based on which VIF unit
    std::queue<uint32_t> fifo; // A first-in, first-out buffer
public:
    VectorInterface(VectorUnit* vu, int id) : vu(vu), id(id) 
    {
        if (id)
            fifo_size = 64;
        else
            fifo_size = 32;
    }

    void write_fifo(uint32_t data);

    void write(uint32_t addr, uint32_t data);
};