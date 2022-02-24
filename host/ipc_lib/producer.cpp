#include "producer.hpp"

#include <doomtime_ipc.h>
#include <doomtime_shm_interface.hpp>

#include <signal.h>
#include <fmt/core.h>

using namespace doomtime::ipc::producer;
using namespace doomtime::ipc;

producer_t& producer_t::get()
{
    static producer_t inst;
    return inst;
}
namespace {
void sigint_handler(int)
{
    producer_t::get().run.store(false);
    signal(SIGINT, nullptr);
}
}

producer_t::producer_t()
{
    bi::shared_memory_object::remove(memory_name);
    shm_ = std::make_unique<bi::shared_memory_object>(
        bi::create_only, memory_name, bi::read_write);
    shm_->truncate(sizeof(shared_memory_buffer));
    region_ = std::make_unique<bi::mapped_region>(*shm_, bi::read_write);
    void * addr = region_->get_address();
    data_ = new (addr) shared_memory_buffer;
    signal(SIGINT, sigint_handler);
    fmt::print("shm inited, waiting for client..\n");
}

producer_t::~producer_t() {
    data_->disconnected = true;
    data_ = nullptr;
    region_.reset();
    shm_.reset();
    bi::shared_memory_object::remove(memory_name);
}

void producer_t::set_palettes(const uint8_t* pals, size_t pal_count)
{
    if (pal_count > data_->palettes.size())
    {
        fmt::print(stderr, "invalid palette count: {}\n", pal_count);
        exit(10);
    }
    std::memcpy(data_->palettes.data(), pals, pal_count * data_->palettes[0].size());
    data_->palette_count = pal_count;
}

void producer_t::frame_ready(const uint8_t* pixels)
{
    std::memcpy(
        data_->frame.data(),
        pixels,
        data_->frame.size());
    data_->nstored.post();
}

void producer_t::frame_rendered()
{
    if (!wait(data_->nempty))
    {
        fmt::print(stderr, "frame sync timeout\n");
        exit_(1);
    }
    if (!run.load())
        disconnected();

}

void producer_t::disconnected()
{
    if (!data_->disconnected)
    {
        data_->disconnected = true;
        data_->nstored.post();
        wait(data_->nempty);
        fmt::print("exiting\n");
        exit_(0);
    }
}

void producer_t::palette_changed(uint8_t pal_idx)
{
    if (pal_idx < 14)
        data_->palette_index.store(pal_idx);
    else
        throw std::range_error("palette index out of range");
}

void doomtime_ipc_set_palettes(const uint8_t* pals, size_t pal_count)
{
    producer_t::get().set_palettes(pals, pal_count);
}

void doomtime_ipc_frame_ready(const uint8_t* pixels)
{
    producer_t::get().frame_ready(pixels);
}

void doomtime_ipc_frame_rendered()
{
    producer_t::get().frame_rendered();
}

void doomtime_ipc_palette_changed(uint8_t pal_idx)
{
    producer_t::get().palette_changed(pal_idx);
}

void doomtime_ipc_disconnected()
{
    producer_t::get().disconnected();
}
