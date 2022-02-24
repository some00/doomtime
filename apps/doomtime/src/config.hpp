#pragma once
#include <cstdint>
#include <climits>

constexpr auto BPP = 12;
constexpr auto DISPLAY_WIDTH = 240;
constexpr auto DISPLAY_HEIGHT = 240;
constexpr auto SCREEN_HEIGHT = 96;
constexpr auto SCREEN_WIDTH = 192;
constexpr auto ORIGINAL_SCREEN_WIDTH = 48;
constexpr auto ORIGINAL_SCREEN_HEIGHT = 48;

constexpr auto PALETTE_COMPONENTS = 3;
constexpr auto NUM_PALETTE_COLORS = 256;
using pal_t = uint8_t[NUM_PALETTE_COLORS * BPP / CHAR_BIT];
using pal_input_t = uint8_t[ORIGINAL_SCREEN_HEIGHT];
using pal_output_t = uint8_t[SCREEN_HEIGHT * BPP / CHAR_BIT];
constexpr auto RING_BUFFER_SIZE = 100;
