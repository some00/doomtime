#ifdef ARCH_sim
#include <hal/hal_spi.h>
int hal_spi_txrx(int spi_num, void *txbuf, void *rxbuf, int cnt)
{
    return 0;
}
int hal_spi_config(int spi_num, struct hal_spi_settings *psettings)
{
    return 0;
}
int hal_spi_set_txrx_cb(int spi_num, hal_spi_txrx_cb txrx_cb, void *arg)
{
    return 0;
}

int hal_spi_init_hw(uint8_t spi_num, uint8_t spi_type,
                    const struct hal_spi_hw_settings *cfg)
{
    return 0;
}
int hal_spi_enable(int spi_num)
{
    return 0;
}
int hal_spi_txrx_noblock(int spi_num, void *txbuf, void *rxbuf, int cnt)
{
    return 0;
}
#endif
