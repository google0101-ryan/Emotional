// (c) Copyright 2022 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <bits/stdint-uintn.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#define _OPEN_SYS_ITOA_EXT
#include <stdlib.h>

typedef union
{
    unsigned __int128 u128;
    uint64_t u64[2];
    uint32_t u32[4];
    uint16_t u16[8];
    uint8_t u8[16];
} uint128_t;

inline char* print_128(uint128_t s)
{
    static char ret[512];

    memset(ret, 0, 512);

    sprintf(ret, "%016lx %016lx", s.u64[1], s.u64[0]);

    return ret;
}