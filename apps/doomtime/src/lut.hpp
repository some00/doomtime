#pragma once
#include "config.hpp"
#include <cassert>
#include <cstdint>

inline void lut(const pal_t& palette, const uint8_t* input, pal_output_t& output)
{
    static_assert(BPP == 12, "only rgb444 is supported");

    uint8_t idi = 0;
    uint8_t ido = 0;
    for(; idi < sizeof(pal_input_t); ++idi, ido += 3)
    {
        const uint16_t pal_idx = input[idi] * BPP / CHAR_BIT;
        uint8_t r, g, b;
        if (pal_idx % 3 == 0)
        {
            r = (palette[pal_idx + 0] & 0xf0) >> 4;
            g = (palette[pal_idx + 0] & 0x0f) >> 0;
            b = (palette[pal_idx + 1] & 0xf0) >> 4;
        }
        else
        {
            r = (palette[pal_idx + 0] & 0x0f) >> 0;
            g = (palette[pal_idx + 1] & 0xf0) >> 4;
            b = (palette[pal_idx + 1] & 0x0f) >> 0;
        }
        output[ido + 0] = (r << 4) | (g << 0);
        output[ido + 1] = (b << 4) | (r << 0);
        output[ido + 2] = (g << 4) | (b << 0);
    }
}
