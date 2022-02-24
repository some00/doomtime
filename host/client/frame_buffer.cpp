#include "exceptions.hpp"
#include "frame_buffer.hpp"

#include <chrono>
#include <stdexcept>

#include <opencv2/core.hpp>

#include <fmt/core.h>

using namespace doomtime::client;
using namespace doomtime;
using namespace std::literals::chrono_literals;

frame_buffer_t::frame_buffer_t(
    size_t stack_init_size, const to_packed_rgb_444_t& to_packed_rgb_444)
: stack_{stack_init_size}
, to_packed_rgb_444_{to_packed_rgb_444}
{
}

void frame_buffer_t::push(const ipc::frame_t& input_array, uint8_t pal_idx, const offset_t& offset)
{
    const auto [X, Y] = offset;
    cv::Mat input_matrix(
        cv::Size(ipc::WIDTH, ipc::HEIGHT),
        CV_8UC1,
        // won't change it, swearsies
        const_cast<uint8_t*>(input_array.data())
    );
    cv::Mat cropped(input_matrix, cv::Range(Y, Y + HEIGHT), cv::Range(X, X + WIDTH * 2));
    cv::Mat transposed(cv::Size(WIDTH, HEIGHT), CV_8UC1);
    cv::transpose(cropped, transposed);
    packets_t pkts;
    cv::Mat output_matrix(cv::Size(HEIGHT, WIDTH), CV_8UC1, pkts[0].data(), STEP);
    for (unsigned i = 0; i < WIDTH * 2; i += 2)
        transposed.row(i).copyTo(output_matrix.row(i / 2));
    for (auto& pkt : pkts)
    {
        for (unsigned i = 0; i < COL_PER_PACKET; ++i)
            pkt[(i + 1) * STEP - 1] = pal_idx;
        pkt[pkt.size() - 1] = pal_idx;
    }
    stack_.push(std::move(pkts));
    cv_.notify_one();
}

std::tuple<bool, packets_t> frame_buffer_t::pop(bool wait)
{
    packets_t first;
    if (stack_.empty())
    {
        if (!wait)
            return std::make_tuple(false, first);
        std::unique_lock<std::mutex> lk{mtx_};
        // TODO first should timeout?
        if (!cv_.wait_for(lk, 3s, [this] () -> bool {
            return !stack_.empty() || !run_;
        }))
            throw frame_timeout_error("frame buffer");
        if (!run_)
            return std::make_tuple(false, first);
    }
    bool flag = true;
    stack_.consume_all_atomic([&first, &flag] (const packets_t& pkts) {
        if (flag)
        {
            first = pkts;
            flag = false;
        }
    });
    return std::make_tuple(true, first);
}

void frame_buffer_t::stop()
{
    std::unique_lock<std::mutex> lk{mtx_};
    run_ = false;
    cv_.notify_one();
}

const std::vector<uint8_t>& frame_buffer_t::palettes() const
{
    return pals_;
}

void frame_buffer_t::set_palettes(const pals_t& pals)
{
    constexpr auto pal_size = pal_t::RowsAtCompileTime * 12 / 8;
    pals_.resize(pals.size() * pal_size);
    auto p = pals_.data();
    for (const auto& pal : pals)
    {
        auto rv = to_packed_rgb_444_(pal);
        if (rv.size() != pal_size)
            throw std::length_error(
                fmt::format("invalid palette size: {}", rv.size()));
        std::memcpy(p, rv.data(), rv.size());
        p += rv.size();
    }
}
