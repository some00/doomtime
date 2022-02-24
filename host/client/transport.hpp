#pragma once
#include "frame_buffer.hpp"
#include "pid_controller.hpp"

#include <boost/asio.hpp>

namespace doomtime {
namespace client {

namespace ba = boost::asio;
using fp_t = pid_controller_t::fp_t;

struct transport_t
{
    using clk_t = std::chrono::steady_clock;
    transport_t(
        int fd, int pal_fd, frame_buffer_t& fb, std::atomic_bool& run,
        fp_t sp, fp_t kp, fp_t ki, fp_t kd, fp_t initial_fps, bool use_pid
    );
    void run();
private:
    using ec_t = boost::system::error_code;
    using resp_t = unsigned int;

    std::atomic_bool& run_;
    frame_buffer_t& fb_;
    clk_t::duration mv_;
    size_t out_cnt_{0};

    ba::io_context ctxt_;
    ba::local::stream_protocol protocol_;
    ba::local::stream_protocol::socket socket_;
    ba::local::stream_protocol::socket pal_socket_;

    ba::steady_timer rtimer_{ctxt_};
    std::array<resp_t, 1> resp_;
    double avg_{0};
    clk_t::time_point rstart_ = clk_t::now();
    clk_t::time_point rlast_ = clk_t::now();
    pid_controller_t pid_;
    std::function<void(ec_t, size_t)> on_read_;
    std::function<void()> start_read_;
    std::function<void()> start_pid_timer_;
    std::function<void(ec_t)> on_pid_timer_;

    packets_t pkts_;
    size_t frame_progress_{0};
    ba::steady_timer wtimer_{ctxt_};
    std::function<void()> start_write_;
    std::function<void()> start_write_timer_;
    std::function<void(ec_t, size_t)> on_write_;
};

}
}
