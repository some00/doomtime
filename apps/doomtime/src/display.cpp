#include "display.hpp"
#include "lut.hpp"
#include "minimum.hpp"
#include <cassert>

extern "C" {
#include <sysinit/sysinit.h>
#include <os/os.h>
#include <bsp/bsp.h>
#include <hal/hal_gpio.h>
#include <hal/hal_spi.h>
#include <modlog/modlog.h>
#include <os/os_sem.h>
}
#include <cstring>
#include <atomic>

void sem_init(os_sem& sem, uint16_t tokens)
{
    auto rc = os_sem_init(&sem, tokens);
    assert(rc == OS_OK);
}

constexpr auto SPI_NUM = 0;
constexpr int PIN_LCD_DATA_COMMAND = 18;
constexpr int PIN_LCD_RESET = 26;
constexpr int PIN_LCD_CS = 25;
constexpr int PIN_SPI_SCK = 2;
constexpr int PIN_SPI_MOSI = 3;
constexpr int PIN_SPI_MISO = 4;
constexpr int PIN_LCD_BACKLIGHT_LOW = 14;
constexpr int PIN_LCD_BACKLIGHT_MEDIUM = 22;
constexpr int PIN_LCD_BACKLIGHT_HIGH = 23;
constexpr int PIN_MOT = 16;

constexpr uint16_t ROW_OFFSET = DISPLAY_HEIGHT / 2 - SCREEN_HEIGHT / 2;
constexpr uint16_t COL_OFFSET = DISPLAY_WIDTH / 2 - SCREEN_WIDTH / 2;
constexpr uint16_t COL_ZERO = 0;
constexpr uint16_t ROW_ZERO = 0;
constexpr bool FPS_ON_MOT_PIN = MYNEWT_VAL(DOOMTIME_FPS_ON_MOT_PIN);



enum class CMD : uint8_t
{
    SOFT_RST = 0x01,
    SLEEP_OUT = 0x11,
    COLMOD = 0x3a,
    DISPON = 0x29,
    RAMWR = 0x2c,
    CASET = 0x2a,
    RASET = 0x2b,
    INVON = 0x21,
    MADCTL = 0x36,
};

enum RGB_INTERFACE_COLOR_FORMAT
{
    CF_65K = 0x05,
    CF_262K = 0x06,
};

enum COLOR_INTERFACE_COLOR_FORMAT
{
    RGB444 = 0x03,
    RGB565 = 0x05,
    RGBB888 = 0x06,
};

display_t& get_disp(void* arg)
{
    return *reinterpret_cast<display_t*>(arg);
}

void fill_begin_end(uint16_t begin, uint16_t end, uint8_t (&arr)[4])
{
    arr[0] = begin >> 8;
    arr[1] = begin & 0xff;
    arr[2] = end >> 8;
    arr[3] = end & 0xff;
}

void init_gpio()
{
    int rc;
    rc = hal_gpio_init_out(PIN_LCD_BACKLIGHT_LOW, 1);
    assert(rc == 0);
    rc = hal_gpio_init_out(PIN_LCD_BACKLIGHT_MEDIUM, 0);
    assert(rc == 0);
    rc = hal_gpio_init_out(PIN_LCD_BACKLIGHT_HIGH, 1);
    assert(rc == 0);
    rc = hal_gpio_init_out(PIN_LCD_DATA_COMMAND, 1);
    assert(rc == 0);
    rc = hal_gpio_init_out(PIN_LCD_RESET, 1);
    assert(rc == 0);
    rc = hal_gpio_init_out(PIN_LCD_CS, 1);
    assert(rc == 0);
    if (FPS_ON_MOT_PIN)
    {
        rc = hal_gpio_init_out(PIN_MOT, 1);
        assert(rc == 0);
    }
}

void spi_tx_cmd(CMD cmd)
{
    hal_gpio_write(PIN_LCD_DATA_COMMAND, 0);
    uint8_t buf = static_cast<uint8_t>(cmd);
    int rc = hal_spi_txrx(SPI_NUM, &buf, NULL, 1);
    assert(rc == 0);
}

template<size_t N>
void spi_tx_data(const uint8_t (&data)[N])
{
    hal_gpio_write(PIN_LCD_DATA_COMMAND, 1);
    int rc = hal_spi_txrx(SPI_NUM, const_cast<uint8_t*>(data), nullptr, N);
    assert(rc == 0);
}

void spi_tx_data(const uint8_t data)
{
    uint8_t temp[] = {data};
    spi_tx_data(temp);
}

void hw_rst()
{
    hal_gpio_write(PIN_LCD_RESET, 0);
    os_time_delay(100);
    hal_gpio_write(PIN_LCD_RESET, 1);
}

void sw_rst()
{
    hal_gpio_write(PIN_LCD_CS, 0);
    hal_gpio_write(PIN_LCD_DATA_COMMAND, 0);
    spi_tx_cmd(CMD::SOFT_RST);
}

void sleep_out()
{
    spi_tx_cmd(CMD::SLEEP_OUT);
    os_time_delay(50);
}

void color_mode()
{
    spi_tx_cmd(CMD::COLMOD);
    spi_tx_data(CF_65K << 4 | RGB444);
    os_time_delay(100);
}

void inversion_on()
{
    spi_tx_cmd(CMD::INVON);
}

void display_on()
{
    spi_tx_cmd(CMD::DISPON);
}

void set_memory_data_access()
{
    const uint8_t data =
        0 << 7 | // MY
        0 << 6 | // MX
        1 << 5 | // MV
        0 << 4 | // ML
        0 << 3 | // BGR
        0 << 2 ; // MH
    spi_tx_cmd(CMD::MADCTL);
    spi_tx_data(data);
}

void init_caset_raset()
{
    uint8_t params[4];
    spi_tx_cmd(CMD::CASET);
    fill_begin_end(ROW_ZERO + ROW_OFFSET, ROW_ZERO + ROW_OFFSET + SCREEN_HEIGHT - 1, params);
    spi_tx_data(params);
    spi_tx_cmd(CMD::RASET);
    fill_begin_end(COL_ZERO + COL_OFFSET, COL_ZERO + COL_OFFSET + SCREEN_WIDTH, params);
    spi_tx_data(params);
}

void display_t::init()
{
    output_head_ = 0;
    output_tail_ = 0;
    state_ = state_t::RAMWR;
    col_count_ = 0;
    col_acquired_ = false;
    reset_.test_and_set();
    std::memset(palette_, 0x00, sizeof(palette_));
    pal_progress_ = 0;

    sem_init(spi_sem_, 0);
    init_spi();
    init_gpio();
    hw_rst();
    sw_rst();
    sleep_out();
    color_mode();
    inversion_on();
    display_on();
    set_memory_data_access();
    clear_screen();
    init_caset_raset();

    sem_init(producer_, RING_BUFFER_SIZE);
    sem_init(consumer_, 0);
    init_caset_raset();
    int rc = os_task_init(
        &display_task_, "display_task", s_display_task_handler, this, DISPLAY_TASK_PRIO,
        OS_WAIT_FOREVER, display_task_stack, DISPLAY_TASK_STACK_SIZE);
    assert(OS_OK == rc);
}

void display_t::init_spi()
{
    hal_spi_hw_settings spi_hw_settings = {
        .pin_sck=PIN_SPI_SCK,
        .pin_mosi=PIN_SPI_MOSI,
        .pin_miso=PIN_SPI_MISO,
        .pin_ss=PIN_LCD_CS
    };
    hal_spi_settings spi_settings = {
        .data_mode=HAL_SPI_MODE3,
        .data_order=HAL_SPI_MSB_FIRST,
        .word_size=HAL_SPI_WORD_SIZE_8BIT,
        .baudrate=8000
    };
    int rc;
    rc = hal_spi_init_hw(SPI_NUM, HAL_SPI_TYPE_MASTER, &spi_hw_settings);
    assert(rc == 0);
    rc = hal_spi_config(SPI_NUM, &spi_settings);
    assert(rc == 0);
    rc = hal_spi_set_txrx_cb(SPI_NUM, &s_spi_txrx_callback, this);
    assert(rc == 0);
    rc = hal_spi_enable(SPI_NUM);
    assert(rc == 0);
}

void display_t::clear_screen()
{
    uint8_t params[4];

    spi_tx_cmd(CMD::CASET);
    fill_begin_end(COL_ZERO, DISPLAY_HEIGHT, params);
    spi_tx_data(params);

    spi_tx_cmd(CMD::RASET);
    fill_begin_end(ROW_ZERO, DISPLAY_WIDTH, params);
    spi_tx_data(params);

    spi_tx_cmd(CMD::RAMWR);

    hal_gpio_write(PIN_LCD_DATA_COMMAND, 1);
    for (unsigned i = 0; i < 5; ++i)
    {
        for (unsigned j = 0; j < sizeof(background_) / sizeof(background_[0]); ++j)
        {
            for (unsigned k = 0; k < 5; ++k)
            {
                auto rc = hal_spi_txrx(
                    SPI_NUM,
                    const_cast<uint8_t*>(background_[j]),
                    nullptr,
                    sizeof(background_[j])
                );
                assert(rc == 0);
            }
        }
    }
}

void display_t::s_spi_txrx_callback(void* arg, int len)
{
    get_disp(arg).spi_txrx_callback(len);
}

void display_t::s_display_task_handler(void* arg)
{
    get_disp(arg).task_handler();
}

static uint8_t ramwr_buf[2] = {
    static_cast<uint8_t>(CMD::RAMWR),
    static_cast<uint8_t>(CMD::RAMWR)
};
void display_t::spi_txrx_callback(int len)
{
    int rc;
    switch (state_)
    {
    case state_t::RAMWR:
        hal_gpio_write(PIN_LCD_DATA_COMMAND, 0);
        if (FPS_ON_MOT_PIN)
            hal_gpio_toggle(PIN_MOT);
        rc = hal_spi_txrx_noblock(SPI_NUM, ramwr_buf, nullptr, sizeof(ramwr_buf));
        assert(rc == 0);
        state_ = state_t::FRAME;
        break;
    case state_t::FRAME:
        if (col_count_ == 0)
            hal_gpio_write(PIN_LCD_DATA_COMMAND, 1);
        if (col_count_ % 4 == 0 && !col_acquired_)
        {
            rc = os_sem_pend(&consumer_, 0);
            if (!reset_.test_and_set())
                col_count_ = 0;
            assert(rc == OS_TIMEOUT || rc == OS_OK);
            if (rc == OS_TIMEOUT)
            {
                os_sem_release(&spi_sem_);
                return;
            }
            os_sem_release(&producer_);
            output_tail_ = (output_tail_ + 1) % RING_BUFFER_SIZE;
        }
        rc = hal_spi_txrx_noblock(
            SPI_NUM, output_[output_tail_], nullptr, sizeof(pal_output_t));
        assert(rc == 0);
        col_acquired_ = false;
        if (++col_count_ == SCREEN_WIDTH)
        {
            col_count_ = 0;
            state_ = state_t::RAMWR;
        }
        break;
    }
}

void display_t::task_handler()
{
    int rc;
    while (true)
    {
        os_sem_pend(&consumer_, OS_TIMEOUT_NEVER);
        if (!reset_.test_and_set())
            col_count_ = 0;
        col_acquired_ = true;
        if (state_ == state_t::RAMWR)
        {
            state_ = state_t::FRAME;
            hal_gpio_write(PIN_LCD_DATA_COMMAND, 0);
            if (FPS_ON_MOT_PIN)
                hal_gpio_toggle(PIN_MOT);
            rc = hal_spi_txrx_noblock(SPI_NUM, ramwr_buf, nullptr, sizeof(ramwr_buf));
        }
        else
        {
            col_count_ = (col_count_ + 1) % SCREEN_WIDTH;
            rc = hal_spi_txrx_noblock(
                    SPI_NUM, output_[output_tail_], nullptr, sizeof(pal_output_t));
        }
        assert(rc == 0);
        os_sem_pend(&spi_sem_, OS_TIMEOUT_NEVER);
        output_tail_ = (output_tail_ + 1) % RING_BUFFER_SIZE;
        os_sem_release(&producer_);
    }
}

void display_t::queue_column(const uint8_t* column, uint8_t pal_idx)
{
    os_sem_pend(&producer_, OS_TIMEOUT_NEVER);
    pal_buf_[output_head_] = pal_idx;
    lut(palette_[pal_buf_[output_head_]], column, output_[output_head_]);
    output_head_ = (output_head_ + 1) % RING_BUFFER_SIZE;
    os_sem_release(&consumer_);
}

void display_t::reset()
{
    pal_progress_ = 0;
    reset_.clear();
}

void display_t::write_palette(const uint8_t* pal, uint8_t size)
{
    constexpr auto pal_size = sizeof(palette_);
    size = minimum<uint16_t>(size, pal_size - pal_progress_);
    std::memcpy(palette_[0] + pal_progress_, pal, size);
    pal_progress_ += size;
    pal_progress_ %= pal_size;
}
