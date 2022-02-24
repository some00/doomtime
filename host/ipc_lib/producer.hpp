#pragma once
#include <memory>
#include <functional>

#include "doomtime_shm_interface.hpp"
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace doomtime {
namespace ipc {
namespace producer {

struct producer_t
{
    using exit_func_t = std::function<void(int)>;
    void set_palettes(const uint8_t* pals, size_t pal_count);
    void frame_ready(const uint8_t* pixels);
    void frame_rendered();
    void palette_changed(uint8_t pal_idx);
    void disconnected();
    static producer_t& get();
    std::atomic_bool run{true};
protected:
    producer_t();
    virtual ~producer_t();
    exit_func_t exit_=::exit;
private:
    std::unique_ptr<bi::shared_memory_object> shm_;
    std::unique_ptr<bi::mapped_region> region_;
    shared_memory_buffer* data_ = nullptr;
};

} // producer
} // ipc
} // dootime
