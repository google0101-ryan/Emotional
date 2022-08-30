#pragma once

#include <bits/stdint-uintn.h>
typedef union
{
    uint32_t full;
    struct
    {
    //     uint32_t opcode : 6;
    //     uint32_t rs : 5;
    //     uint32_t rt : 5;
    //     uint32_t rd : 5;
    //     uint32_t sa : 5;
    //     uint32_t func : 6;
        uint32_t func : 6;
        uint32_t sa : 5;
        uint32_t rd : 5;
        uint32_t rt : 5;
        uint32_t rs : 5;
        uint32_t opcode : 6;
    } r_type;
} Opcode;