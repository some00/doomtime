#include "producer.hpp"

#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/eigen.h>

using namespace doomtime::ipc::producer;
namespace py = pybind11;

namespace {
struct tested_producer_t : producer_t
{
public:
    using producer_t::producer_t;
    void set_exit(exit_func_t exit) { exit_ = exit; }
};
struct helper_t : std::enable_shared_from_this<helper_t>
{
    std::unique_ptr<tested_producer_t> producer;
    producer_t::exit_func_t exit;
    helper_t(producer_t::exit_func_t exit) : exit{exit} {}
    tested_producer_t& get()
    {
        if (!producer)
            throw std::runtime_error("use with statement");
        return *producer;
    }
};
}

PYBIND11_MODULE(doomtime_producer, m)
{
    using frame_t = Eigen::Matrix<
        uint8_t, doomtime::ipc::HEIGHT, doomtime::ipc::WIDTH, Eigen::RowMajor>;
    py::class_<helper_t, std::shared_ptr<helper_t>> var(m, "Producer");
    var.def(py::init<producer_t::exit_func_t>());
    var.def("__enter__", [] (helper_t& self) {
        if (self.producer)
            throw std::runtime_error("use with statement");
        self.producer = std::make_unique<tested_producer_t>();
        self.producer->set_exit(self.exit);
        return self.shared_from_this();
    });
    var.def("__exit__", [] (helper_t& self, py::args) {
        self.producer->disconnected();
        self.producer.reset();
    }, py::call_guard<py::gil_scoped_release>());
    var.def("frame", [] (helper_t& self, Eigen::Ref<frame_t> frame) {
        self.get().frame_ready(frame.data());
        self.get().frame_rendered();
    }, py::call_guard<py::gil_scoped_release>(), py::arg("frame"));
    var.def("palette_changed", [] (helper_t& self, uint8_t pal_idx) {
        self.get().palette_changed(pal_idx);
    }, py::arg("pal_idx"));
    var.def("disconnected", [] (helper_t& self) {
        self.get().disconnected();
    }, py::call_guard<py::gil_scoped_release>());
}
