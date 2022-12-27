#pragma once

#include <bits/stdint-uintn.h>

struct Opcode
{
    union
    {
        uint32_t full;
        struct
        { /* Used when polling for the opcode */
            uint32_t : 26;
            uint32_t opcode : 6;
        };
        struct
        {
            uint32_t imm : 16;
            uint32_t rt : 5;
            uint32_t rs : 5;
            uint32_t opcode : 6;
        } i_type;
        struct
        {
            uint32_t target : 26;
            uint32_t opcode : 6;
        } j_type;
        struct
        {
            uint32_t func : 6;
            uint32_t sa : 5;
            uint32_t rd : 5;
            uint32_t rt : 5;
            uint32_t rs : 5;
            uint32_t opcode : 6;
        } r_type;
    };

    uint32_t pc;
    bool is_delay_slot = false;
	bool branch_taken = false;
};