#pragma once
#include <cstdint>
#include "display.hpp"

extern "C" {
#include <host/ble_gatt.h>
#include <host/ble_gap.h>
#include <os/os.h>
}


struct bt_t
{
    void init(display_t& disp);
private:
    display_t* disp_;
    pal_input_t col_buffer_;
    uint8_t packet_buf_[196];

    void advertise();

    static int s_access_frames(
        uint16_t conn_handle, uint16_t attr_handle, ble_gatt_access_ctxt* ctxt, void* arg);
    int access_frames(ble_gatt_access_ctxt* ctxt);

    static int s_access_reset(
         uint16_t conn_handle, uint16_t attr_handle, ble_gatt_access_ctxt* ctxt, void* arg);
    int access_reset(ble_gatt_access_ctxt* ctxt);

    static int s_access_pals(
         uint16_t conn_handle, uint16_t attr_handle, ble_gatt_access_ctxt* ctxt, void* arg);
    int access_pals(ble_gatt_access_ctxt* ctxt);

    static void s_on_sync();
    void on_sync();

    static void s_on_reset(int reason);
    void reset();

    static int s_on_gap_event(ble_gap_event* event, void* arg);
    int on_gap_event(ble_gap_event* event);
};


