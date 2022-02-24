#include "exceptions.hpp"
#include "transport.hpp"
#include "pid_controller.hpp"
#include "unset_run.hpp"
#include "consumer_thread.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/eigen.h>
#include <fmt/core.h>

using namespace doomtime::client;
using fp_t = pid_controller_t::fp_t;

namespace py = pybind11;

namespace  {
void run(
    int fd, int pal_fd, fp_t sp, fp_t kp, fp_t ki, fp_t kd, fp_t initial_fps,
    size_t initial_stack_size, bool use_pid,
    const to_packed_rgb_444_t& to_packed_rgb_444
)
{
    std::atomic_bool run{true};
    unset_run_t ur{run};
    frame_buffer_t fb{initial_stack_size, to_packed_rgb_444};
    consumer_thread_t ct{fb, run};
    ct.start();
    transport_t tr(fd, pal_fd, fb, run, sp, kp, ki, kd, initial_fps, use_pid);
    tr.run();
    ct.join();
}
}

PYBIND11_MODULE(doomtime_client, m)
{
    m.def("run", &run, py::call_guard<py::gil_scoped_release>(),
        py::arg("fd"), py::arg("pal_fd"), py::arg("sp"),
        py::arg("kp"), py::arg("ki"), py::arg("kd"),
        py::arg("initial_fps"), py::arg("initial_stack_size"),
        py::arg("use_pid"), py::arg("to_packed_rgb_444")
    );
    py::register_exception<pending_read_error>(m, "PendingReadError");
    py::register_exception<pending_timer_error>(m, "PendingTimerError");
    py::register_exception<frame_write_error>(m, "FrameWriteError");
    py::register_exception<frame_timer_error>(m, "FrameTimerError");
    py::register_exception<frame_timeout_error>(m, "FrameTimeoutError");
}
