#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <emu/cpu/opcode.h>

float convert(uint32_t val);

class VectorUnit
{
private:
    union
    {
        uint16_t u;
        int16_t s;
    } i_regs[16];

    union
    {
        float f[4];
        uint32_t u[4];
        int32_t s[4];
    } gpr[32];

    union
    {
        float f;
        uint32_t u;
        char padding[16];
    } R, I, Q, P;

    int id = 0;
    uint32_t fbrst;
    uint32_t status;
    uint32_t clip_flags;
    uint32_t cmsar0;

    uint8_t data_mem[0x4000];
    uint8_t instr_mem[0x4000];
public:
    VectorUnit(int id) : id(id) {}
    void Dump();

    template<typename T> T read_data(uint32_t addr)
    {
        if (id == 1)
            return *(T*)&data_mem[addr & 0x3fff];
        return *(T*)&data_mem[addr & 0xfff];
    }

    template<typename T> void write_data(uint32_t addr, uint32_t data)
    {
        if (id == 1)
            *(T*)&data_mem[addr & 0x3fff] = data;
        else
            *(T*)&data_mem[addr & 0xfff] = data;
    }

    template<typename T> T read_code(uint32_t addr)
    {
        if (id == 1)
            return *(T*)&instr_mem[addr & 0x3fff];
        return *(T*)&instr_mem[addr & 0xfff];
    }

    template<typename T> void write_code(uint32_t addr, uint32_t data)
    {
        if (id == 1)
            *(T*)&instr_mem[addr & 0x3fff] = data;
        else
            *(T*)&instr_mem[addr & 0xfff] = data;
    }

    uint32_t cfc(int index)
    {
        if (index < 16)
            return i_regs[index].u;
		return 0;
    }

    void ctc(int index, uint32_t data)
    {
        if (index < 16)
        {
            i_regs[index].u = data;
            return;
        }

        switch (index)
        {
        case 20:
            R.u = data;
            return;
        case 21:
            I.u = data;
            return;
        case 22:
            Q.u = data;
            return;
        }

        printf("[emu/VU%d]: %s: Failed to write cop2 reg %d\n", id, __FUNCTION__, index);
        exit(1);
    }

    void special1(Opcode i);

    uint32_t GetGprU(int index, int field)
    {
        return gpr[index].u[field];
    }

    void SetGprU(int index, int field, uint32_t value)
    {
        gpr[index].u[field] = value;
    }
};