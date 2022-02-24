#include "bt.hpp"
#include "minimum.hpp"
#include <cassert>
#include <cstring>

extern "C" {
#include <os/os.h>
#include <nimble/ble.h>
#include <host/ble_uuid.h>
#include <host/util/util.h>
#include <host/ble_hs.h>
#include <services/gap/ble_svc_gap.h>
#include <modlog/modlog.h>
}

namespace {
bt_t& get_bt(void* arg) {
    return *reinterpret_cast<bt_t*>(arg);
}

// 71d89cd5-80e4-4b68-a09c-482051ab1414
constexpr ble_uuid128_t gatt_svr_svc_doom_uuid = {
    .u={.type=BLE_UUID_TYPE_128},
    .value={
        0x14, 0x14, 0xab, 0x51, 0x20, 0x48,
        0x9c, 0xa0,
        0x68, 0x4b,
        0xe4, 0x80,
        0xd5, 0x9c, 0xd8, 0x71,
    }
};
// 765ffe6c-5b87-4541-a2ef-eb3bdd46a462
constexpr ble_uuid128_t gatt_svr_svc_frames = {
    .u={.type=BLE_UUID_TYPE_128},
    .value={
        0x62, 0xa4, 0x46, 0xdd, 0x3b, 0xeb,
        0xef, 0xa2,
        0x41, 0x45,
        0x87, 0x5b,
        0x6c, 0xfe, 0x5f, 0x76,
    }
};
// 061746ad-f829-4669-a1f7-2f3dec24cf00
constexpr ble_uuid128_t gatt_svr_svc_reset = {
    .u={.type=BLE_UUID_TYPE_128},
    .value={
        0x00, 0xcf, 0x24, 0xec, 0x3d, 0x2f,
        0xf7, 0xa1,
        0x69, 0x46,
        0x29, 0xf8,
        0xad, 0x46, 0x17, 0x06,
    }
};

// 03978775-348d-4956-aa1c-7885468f1be8
constexpr ble_uuid128_t gatt_svr_svc_pals = {
    .u={.type=BLE_UUID_TYPE_128},
    .value={
        0xe8, 0x1b, 0x8f, 0x46, 0x85, 0x78,
        0x1c, 0xaa,
        0x56, 0x49,
        0x8d, 0x34,
        0x75, 0x87, 0x97, 0x03,
    }
};

ble_gatt_chr_def chr_defs[3] = {0};
ble_gatt_svc_def svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_doom_uuid.u,
        .characteristics = chr_defs,
    },
    {0},
};

constexpr ble_gatt_chr_def create_chr_def(
    const ble_uuid_t * uuid,
    ble_gatt_access_fn * access_cb,
    bt_t* arg,
    ble_gatt_chr_flags flags
)
{
    return ble_gatt_chr_def{
        .uuid=uuid,
        .access_cb=access_cb,
        .arg=arg,
        .flags=flags
    };
}

} // namespace anonym

int bt_t::s_access_frames(
    uint16_t conn_handle, uint16_t attr_handle, ble_gatt_access_ctxt* ctxt, void* arg)
{
    (void) conn_handle;
    (void) attr_handle;
    return get_bt(arg).access_frames(ctxt);
}

int bt_t::s_access_reset(
    uint16_t conn_handle, uint16_t attr_handle, ble_gatt_access_ctxt* ctxt, void* arg)
{
    (void) conn_handle;
    (void) attr_handle;
    return get_bt(arg).access_reset(ctxt);
}

int bt_t::s_access_pals(
    uint16_t conn_handle, uint16_t attr_handle, ble_gatt_access_ctxt* ctxt, void* arg)
{
    (void) conn_handle;
    (void) attr_handle;
    return get_bt(arg).access_pals(ctxt);
}

void bt_t::s_on_sync()
{
    get_bt(ble_hs_cfg.store_status_arg).on_sync();
}

void bt_t::s_on_reset(int reason)
{
    MODLOG_DFLT(ERROR, "Resetting BLE state for reason: %d\n", reason);
    get_bt(ble_hs_cfg.store_status_arg).reset();
}

int bt_t::s_on_gap_event(ble_gap_event* event, void* arg)
{
    return get_bt(arg).on_gap_event(event);
}


void bt_t::init(display_t& disp)
{
    disp_ = &disp;

    std::memset(chr_defs, 0, sizeof(chr_defs));
    chr_defs[0] = create_chr_def(
            &gatt_svr_svc_frames.u, s_access_frames, this, BLE_GATT_CHR_F_WRITE_NO_RSP);
    chr_defs[1] = create_chr_def(
            &gatt_svr_svc_reset.u, s_access_reset, this, BLE_GATT_CHR_F_WRITE);
    chr_defs[2] = create_chr_def(
            &gatt_svr_svc_pals.u, s_access_pals, this, BLE_GATT_CHR_F_WRITE_NO_RSP);
    svcs[0].characteristics = chr_defs;
    ble_hs_cfg.reset_cb = s_on_reset;
    ble_hs_cfg.sync_cb = s_on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    ble_hs_cfg.store_status_arg = this;
    int rc = ble_gatts_count_cfg(svcs);
    assert(rc == 0);
    rc = ble_gatts_add_svcs(svcs);
    assert(rc == 0);
}

void bt_t::on_sync()
{
    int rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);
    advertise();
}

void bt_t::advertise()
{
    MODLOG_DFLT(INFO, "start advertise\n");
    uint8_t own_addr_type;
    int rc = ble_hs_id_infer_auto(0, &own_addr_type);

    struct ble_hs_adv_fields fields = {0};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    const char* name = ble_svc_gap_device_name();
    fields.name = reinterpret_cast<uint8_t*>(const_cast<char*>(name));
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;
    rc = ble_gap_adv_set_fields(&fields);
    assert(rc == 0);

    struct ble_gap_adv_params adv_params = {0};
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(
        own_addr_type, nullptr, BLE_HS_FOREVER, &adv_params, s_on_gap_event, this);
}

int bt_t::on_gap_event(ble_gap_event* event)
{
    int rc;
    struct ble_gap_conn_desc desc;
    MODLOG_DFLT(INFO, "gap event %d\n", event->type);
    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        MODLOG_DFLT(INFO, "connection %d\n", event->connect.status);
        if (event->connect.status == 0)
        {
            rc = ble_gap_set_prefered_le_phy(
                event->connect.conn_handle,
                BLE_GAP_LE_PHY_2M_MASK, BLE_GAP_LE_PHY_2M_MASK,
                BLE_GAP_LE_PHY_CODED_ANY);
            MODLOG_DFLT(INFO, "PHY set result %d\n", rc);
        }
        else
            advertise();
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        MODLOG_DFLT(INFO, "disconnected reason=%d\n", event->disconnect.reason);
        advertise();
        break;
    case BLE_GAP_EVENT_CONN_UPDATE:
        MODLOG_DFLT(INFO, "connection updated status=%d\n", event->conn_update.status);
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        MODLOG_DFLT(INFO, "advertise complete reason=%d\n", event->adv_complete.reason);
        advertise();
        break;
    case BLE_GAP_EVENT_ENC_CHANGE:
        MODLOG_DFLT(INFO, "encryption change event status=%d\n", event->enc_change.status);
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        break;
    case BLE_GAP_EVENT_MTU:
        MODLOG_DFLT(INFO, "mtu update event mtu=%d\n", event->mtu.value);
        break;
    case BLE_GAP_EVENT_REPEAT_PAIRING:
        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == 0);
        ble_store_util_delete_peer(&desc.peer_id_addr);
        return BLE_GAP_REPEAT_PAIRING_RETRY;
    case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE:
        rc = ble_gap_conn_find(event->phy_updated.conn_handle, &desc);
        assert(rc == 0);
        break;
    }
    return 0;

}

void bt_t::reset()
{
    disp_->reset();
}

int bt_t::access_frames(ble_gatt_access_ctxt* ctxt)
{
    assert(ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR);
    uint16_t size;
    auto rc = ble_hs_mbuf_to_flat(ctxt->om, packet_buf_, sizeof(packet_buf_), &size);
    assert(rc == 0);
    assert(size == 196);
    for (int i = 0; i < 196 / 49; ++i)
        disp_->queue_column(packet_buf_ + i * 49, packet_buf_[(i + 1) * 49 - 1]);
    return 0;
}

int bt_t::access_reset(ble_gatt_access_ctxt* ctxt)
{
    assert(ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR);
    if (1 != OS_MBUF_PKTLEN(ctxt->om))
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    reset();
    return 0;
}

int bt_t::access_pals(ble_gatt_access_ctxt* ctxt)
{
    assert(ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR);
    uint16_t size;
    auto rc = ble_hs_mbuf_to_flat(ctxt->om, packet_buf_, sizeof(packet_buf_), &size);
    assert(rc == 0);
    assert(size <= sizeof(packet_buf_));
    disp_->write_palette(packet_buf_, size);
    return 0;
}
