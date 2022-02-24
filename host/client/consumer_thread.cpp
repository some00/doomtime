#include "unset_run.hpp"
#include "consumer_thread.hpp"
#include <functional>
#include <algorithm>
#include <fmt/core.h>

struct doomtime::client::shm_remove
{
    ~shm_remove()
    {
        bi::shared_memory_object::remove(doomtime::ipc::memory_name);
    }
};

using namespace doomtime::client;

consumer_thread_t::consumer_thread_t(frame_buffer_t& fb, std::atomic_bool& run)
: fb_{fb}
, run_{run}
{
}

void consumer_thread_t::join()
{
    run_.store(false);
    if (fut_.valid())
        fut_.get();
}

consumer_thread_t::~consumer_thread_t()
{
    join();
    if (data_)
        data_->nempty.post();
}

void consumer_thread_t::start()
{
    shm_remove_ = std::make_unique<shm_remove>();
    shm = std::make_unique<bi::shared_memory_object>(
        bi::open_only, ipc::memory_name, bi::read_write);
    region = std::make_unique<bi::mapped_region>(*shm, bi::read_write);
    data_ = reinterpret_cast<ipc::shared_memory_buffer*>(region->get_address());
    fut_ = std::async(std::launch::async, std::bind(&consumer_thread_t::loop, this));
}

void consumer_thread_t::loop()
{
    unset_run_t ur{run_};

    ipc::frame_t buf;
    uint8_t pal_idx;
    bool first = true;

    while (run_.load())
    {
        if (!ipc::wait(data_->nstored))
        {
            fb_.stop();
            fmt::print(stderr, "consumer frame timeout\n");
            return;
        }
        if (data_->disconnected)
        {
            fb_.stop();
            fmt::print("disconnected\n");
            return;
        }
        if (first)
        {
            if (data_->palette_count == 0)
            {
                fb_.stop();
                fmt::print(stderr, "palette hasn't arrived before first frame\n");
                return;
            }
            if (data_->windowx < 0 || data_->windowy < 0)
            {
                fb_.stop();
                fmt::print(stderr, "invalid window offset\n");
                return;
            }
            fmt::print("palette count: {}\n", data_->palette_count);
            pals_t pals;
            std::transform(
                data_->palettes.begin(), data_->palettes.begin() + data_->palette_count,
                std::back_inserter(pals), [] (const auto& x) { return pal_t(x.data()); }
            );
            fb_.set_palettes(pals);
            first = false;
        }
        buf = data_->frame;
        pal_idx = data_->palette_index;
        data_->nempty.post();
        fb_.push(buf, pal_idx, {data_->windowx, data_->windowy});
    }
}
