#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include "lut.hpp"
#include <Eigen/Dense>

using pal_matrix_t = Eigen::Matrix<
    uint8_t, NUM_PALETTE_COLORS * BPP / CHAR_BIT, 1>;
using pal_input_matrix_t = Eigen::Matrix<uint8_t, ORIGINAL_SCREEN_HEIGHT, 1>;
using pal_output_matrix_t = Eigen::Matrix<uint8_t, SCREEN_HEIGHT * BPP / CHAR_BIT, 1>;

struct sizes_t {};

template<typename T>
std::tuple<size_t, size_t> get_shape(pybind11::object)
{
    return std::make_tuple(T::RowsAtCompileTime, T::ColsAtCompileTime);
}


PYBIND11_MODULE(test_lut_module, m)
{
    pybind11::class_<sizes_t>(m, "shapes")
    .def_property_readonly_static("pal", &get_shape<pal_matrix_t>)
    .def_property_readonly_static("input", &get_shape<pal_input_matrix_t>)
    .def_property_readonly_static("output", &get_shape<pal_output_matrix_t>)
    ;
    m.def("lut", [] (const pal_matrix_t& pal_matrix, const pal_input_matrix_t& input_matrix) {
        pal_t pal;
        static_assert(
            sizeof(pal) ==
            pal_matrix_t::RowsAtCompileTime * pal_matrix_t::ColsAtCompileTime,
            "invalid pal size");
        std::memcpy(pal, pal_matrix.data(), sizeof(pal));
        pal_input_t input;
        static_assert(
            sizeof(input) ==
                pal_input_matrix_t::RowsAtCompileTime * pal_input_matrix_t::ColsAtCompileTime,
            "invalid input size");
        std::memcpy(input, input_matrix.data(), sizeof(input));
        pal_output_t output;
        lut(pal, input, output);
        pal_output_matrix_t rv;
        static_assert(
            sizeof(output) ==
                pal_output_matrix_t::RowsAtCompileTime * pal_output_matrix_t::ColsAtCompileTime,
            "invalid output size");
        std::memcpy(rv.data(), output, sizeof(output));
        return rv;
    });
}
