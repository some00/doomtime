#pragma once
#include <array>
#include <atomic>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace doomtime {
namespace ipc {
constexpr int WIDTH = 320;
constexpr int HEIGHT = 200;
namespace bi = boost::interprocess;
using frame_t = std::array<uint8_t, WIDTH * HEIGHT>;
using palettes_t = std::array<std::array<uint8_t, 256 * 3>, 24>; // 256 color 24 for hexen
struct shared_memory_buffer
{
    bi::interprocess_semaphore nempty{0};
    bi::interprocess_semaphore nstored{0};
    frame_t frame;
    std::atomic_uint8_t palette_index{0};
    bool disconnected{false};
    palettes_t palettes;
    size_t palette_count{0};
};
constexpr auto memory_name = "doomtime_ipc_shared_memory";
inline bool wait(bi::interprocess_semaphore& sem) {
   using namespace boost::posix_time;
   return sem.timed_wait(second_clock::universal_time() + seconds(10));
}
} // namespace ipc
} // namespace doomtime
