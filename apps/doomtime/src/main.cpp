#include <assert.h>
#include <string.h>
#include "bt.hpp"
#include "display.hpp"

extern "C" {
#include <sysinit/sysinit.h>
#include <os/os.h>
#include <bsp/bsp.h>
#include <hal/hal_gpio.h>
}

static bt_t bt;
static display_t disp;

int main(int argc, char **argv)
{
    int rc;
    sysinit();
    disp.init();
    bt.init(disp);
    while (true)
    {
        os_eventq_run(os_eventq_dflt_get());
    }
    assert(false);
    return rc;
}

