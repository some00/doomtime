#include "exceptions.hpp"
#include "transport.hpp"
#include "pid_controller.hpp"
#include "unset_run.hpp"
#include <numeric>
#include <fmt/core.h>
#include <thread>

using namespace doomtime::client;
using namespace std::literals::chrono_literals;
using namespace std::chrono;
using prec_t = std::chrono::microseconds;


namespace {
template<class Rep, class Period>
auto cast(const duration<Rep, Period>& d) { return duration_cast<prec_t>(d); }
}

transport_t::transport_t(
    int fd, int pal_fd, frame_buffer_t& fb, std::atomic_bool& run,
    fp_t sp, fp_t kp, fp_t ki, fp_t kd, fp_t initial_fps, bool use_pid
)
: run_{run}
, fb_{fb}
, mv_{cast(cast(1s) / std::clamp<fp_t>(initial_fps, 2, 35))}
, socket_{ctxt_, protocol_, fd}
, pal_socket_{ctxt_, protocol_, pal_fd}
, pid_{sp, kp, ki, kd, cast(1s).count() / fp_t(35.0), fp_t(cast(0.5s).count())}
{

    if (use_pid)
    {
        start_read_ = [this] {
            ba::async_read(socket_, ba::buffer(resp_), on_read_);
        };
        on_read_ = [this] (ec_t ec, size_t s) {
            if (ec)
                throw pending_read_error(ec.message());
            if (s != sizeof(resp_t))
                throw pending_read_error(fmt::format("invalid size: {}", s));
            if (!run_.load())
            {
                ctxt_.stop();
                return;
            }
            const auto now = clk_t::now();
            const auto dt = cast(now - rlast_).count();
            const fp_t x = fp_t(resp_[0]) * dt;
            avg_ += x;
            rlast_ = now;
            start_read_();
        };
        start_pid_timer_ = [this] {
            rtimer_.expires_after(1s);
            rtimer_.async_wait(on_pid_timer_);
        };
        on_pid_timer_ = [this] (ec_t ec) {
            if (ec)
                throw pending_timer_error(ec.message());
            if (!run_.load())
            {
                ctxt_.stop();
                return;
            }
            const auto now = clk_t::now();
            avg_ /= fp_t(cast(now - rstart_).count());
            const auto mv = pid_.compute(avg_);
            fmt::print(
                "fps: {:02.2f} avg: {:.5f} mv: {:06.2f}\n",
                cast(1s).count() / fp_t(cast(mv_).count()), avg_, mv
            );
            mv_ = prec_t(uint64_t(mv));
            avg_ = 0;
            out_cnt_ = 0;
            rstart_ = now;
            start_pid_timer_();
        };
    }
    else
    {
        start_read_ = [] {};
        start_pid_timer_ = [] {};
    }
    start_write_ = [this] {
        if (!run_.load())
        {
            ctxt_.stop();
            return;
        }
        ba::async_write(socket_, ba::buffer(pkts_[frame_progress_++]), on_write_);
    };
    on_write_ = [this] (ec_t ec, size_t size) {
        if (ec)
            throw frame_write_error(ec.message());
        if (size != pkts_[0].size())
            throw frame_write_error(fmt::format("invalid size: {}", size));
        if (!run_.load())
        {
            ctxt_.stop();
            return;
        }
        if (frame_progress_ < pkts_.size())
            start_write_();
        else
            start_write_timer_();
    };
    start_write_timer_ = [this] {
        if (!run_.load())
        {
            ctxt_.stop();
            return;
        }
        wtimer_.expires_after(mv_);
        wtimer_.async_wait([this] (ec_t ec) {
            if (ec)
                throw frame_timer_error(ec.message());
            frame_progress_ = 0;
            auto [ok, t_pkts] = fb_.pop(false);
            if (ok)
                pkts_ = std::move(t_pkts);
            start_write_();
        });
    };
}

void transport_t::run()
{
    unset_run_t ur{run_};
    auto [ok, t_pkts] = fb_.pop();
    if (!ok)
        return;
    pkts_ = std::move(t_pkts);

    fmt::print("packed palettes size: {}\n", fb_.palettes().size());
    const auto& pals = fb_.palettes();
    auto p = pals.data();
    auto left = pals.size();
    while (left > 0)
    {
        const auto size = std::min<size_t>(140, left);
        const auto written = ba::write(pal_socket_, ba::const_buffer(p, size));
        if (written != size)
            throw std::runtime_error(
                fmt::format("palette write size: {}", written));
        left -= size;
        p += size;
    }

    start_pid_timer_();
    start_read_();
    start_write_timer_();
    ctxt_.run();
}
