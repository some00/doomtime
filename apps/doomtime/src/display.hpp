#pragma once
#include "config.hpp"
#include <climits>
#include <cstdint>
#include <atomic>
extern "C" {
#include <os/os.h>
}

constexpr auto DISPLAY_TASK_PRIO = OS_TASK_PRI_HIGHEST + 1;
constexpr auto DISPLAY_TASK_STACK_SIZE = 1024;

struct display_t
{
    void init();
    void write_frame(uint64_t frame_cnt);
    void queue_column(const uint8_t* column, uint8_t pal_idx);
    void reset();
    void write_palette(const uint8_t* pal, uint8_t size);
private:
    void init_spi();
    void clear_screen();
    static void s_spi_txrx_callback(void* arg, int len);
    void spi_txrx_callback(int len);
    os_sem spi_sem_;
    static void s_display_task_handler(void* arg);
    void task_handler();
    os_task display_task_;
    os_stack_t display_task_stack[DISPLAY_TASK_STACK_SIZE];
    pal_t palette_[28];
    uint16_t pal_progress_;
    // generated with scripts/background.py
    static const uint8_t background_[48][48 * BPP / CHAR_BIT];
    pal_input_t input_;
    os_sem producer_;
    os_sem consumer_;
    pal_output_t output_[RING_BUFFER_SIZE];
    uint8_t pal_buf_[RING_BUFFER_SIZE];
    uint8_t output_head_;
    uint8_t output_tail_;
    enum class state_t
    {
        RAMWR,
        FRAME,
    };
    volatile state_t state_;
    volatile uint8_t col_count_;
    volatile bool col_acquired_;
    std::atomic_flag reset_;
};
