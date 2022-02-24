#pragma once
#include "doomtime_shm_interface.hpp"
#include "frame_buffer.hpp"

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <future>
#include <atomic>

namespace doomtime {
namespace client {

namespace bi = boost::interprocess;

struct shm_remove;

struct consumer_thread_t
{
    explicit consumer_thread_t(frame_buffer_t& fb, std::atomic_bool& run);
    void start();
    void join();
    uint8_t frames_per_second() const { return fps_.load(); }
    uint8_t queued_per_second() const { return qps_.load(); }
    consumer_thread_t(const consumer_thread_t&) = delete;
    consumer_thread_t(consumer_thread_t&&) = delete;
    ~consumer_thread_t();
private:
    void loop();

    frame_buffer_t& fb_;
    std::atomic_bool& run_;
    std::atomic_uint8_t fps_{0};
    std::atomic_uint8_t qps_{0};
    std::future<void> fut_;
    ipc::shared_memory_buffer* data_ = nullptr;
    std::unique_ptr<bi::mapped_region> region;
    std::unique_ptr<bi::shared_memory_object> shm;
    std::unique_ptr<shm_remove> shm_remove_;
};

}
}
