#pragma once
#include "doomtime_shm_interface.hpp"

#include <tuple>
#include <array>
#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <vector>
#include <functional>

#include <boost/lockfree/stack.hpp>

#include <Eigen/Dense>

namespace doomtime {
namespace client {

constexpr auto WIDTH = 48;
constexpr auto HEIGHT = 48;
constexpr auto COL_PER_PACKET = 4;
constexpr auto STEP = HEIGHT + sizeof(uint8_t);
constexpr auto MTU = COL_PER_PACKET * STEP;

using packet_t = std::array<uint8_t, MTU>;
using packets_t = std::array<packet_t, WIDTH / COL_PER_PACKET>;
using pal_t = Eigen::Matrix<uint8_t, 256, 3, Eigen::RowMajor>;
using pals_t = std::vector<pal_t>;
using to_packed_rgb_444_t = std::function<std::string(const pal_t&)>;

struct frame_buffer_t
{
    static constexpr auto Y = 60;
    static constexpr auto X = 112;

    frame_buffer_t(
        size_t stack_init_size, const to_packed_rgb_444_t& to_packed_rgb_444);
    void push(const ipc::frame_t& frame, uint8_t pal_idx);
    std::tuple<bool, packets_t> pop(bool wait=true);
    void stop();
    const std::vector<uint8_t>& palettes() const;
    void set_palettes(const pals_t& pals);
private:
    std::mutex mtx_;
    std::condition_variable cv_;
    boost::lockfree::stack<packets_t> stack_;
    bool run_{true};
    to_packed_rgb_444_t to_packed_rgb_444_;
    std::vector<uint8_t> pals_;
};

}
}
