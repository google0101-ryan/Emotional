#pragma once

#include <cstdint>
#include <util/uint128.h>

namespace VectorUnit
{

void WriteDataMem128(int vector, uint32_t addr, uint128_t data);
void WriteDataMem32(int vector, uint32_t addr, uint32_t data);

void WriteCodeMem128(int vector, uint32_t addr, uint128_t data);
void WriteCodeMem64(int vector, uint32_t addr, uint64_t data);

void Dump();

namespace VU0
{

struct VectorState
{
    uint32_t fbrst = 0;
    uint16_t vi[16] = {0};

    uint32_t status;

    union
    {
        uint32_t val;
        struct
        {
            uint32_t pos_x_0 : 1;
            uint32_t neg_x_0 : 1;
            uint32_t pos_y_0 : 1;
            uint32_t neg_y_0 : 1;
            uint32_t pos_z_0 : 1;
            uint32_t neg_z_0 : 1;
            uint32_t : 26;
        };
    } clipping;

    uint32_t r;
    float i, q;

    uint16_t cmsar0;

    union
    {
        uint128_t u128;
        float components[4];
        struct
        {
            float w;
            float z;
            float y;
            float x;
        };
        struct
        {
            uint32_t wi;
            uint32_t zi;
            uint32_t yi;
            uint32_t xi;
        };
    } vf[32], acc;
};

extern VectorState vu0_state;

// VU0 specific stuff, like COP2 opcodes
uint32_t ReadControl(int index);
void WriteControl(int index, uint32_t data);

__uint128_t ReadReg(int index);
void WriteReg(int index, __uint128_t val);

void LQC2(uint32_t instr);
void SQC2(uint32_t instr);

// Special 1
void Vmaddz(uint32_t instr); // 0x0a
void Vmaddw(uint32_t instr); // 0x0b
void Vsub(uint32_t instr); // 0x2c
void Viadd(uint32_t instr); // 0x30

// Special2
void Vmaddax(uint32_t instr); // 0x08
void Vmadday(uint32_t instr); // 0x09
void Vmaddaz(uint32_t instr); // 0x0A
void Vmulax(uint32_t instr); // 0x18
void Vmulaw(uint32_t instr); // 0x1B
void Vclipw(uint32_t instr); // 0x1F
void Vsqi(uint32_t instr); // 0x35
void Viswr(uint32_t instr); // 0x3f

}

}