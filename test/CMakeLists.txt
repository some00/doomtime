pybind11_add_module(test_lut_module test_lut.cpp)
target_link_libraries(test_lut_module PRIVATE Eigen3::Eigen)
target_include_directories(test_lut_module PRIVATE ${doomtime_src})

add_test(NAME test_lut COMMAND ${Python3_EXECUTABLE} -m pytest -vvv ${CMAKE_CURRENT_SOURCE_DIR}/test_lut.py)
set_tests_properties(test_lut PROPERTIES
    ENVIRONMENT PYTHONPATH=$<SHELL_PATH:${CMAKE_CURRENT_BINARY_DIR}$<SEMICOLON>${python_src}>
)
