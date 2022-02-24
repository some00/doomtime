cmake_minimum_required(VERSION 3.18)
project(doomtime)

option(ENABLE_TESTING "Enable testing" ON)

include(FetchContent)
FetchContent_Declare(pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.9.1
)
FetchContent_MakeAvailable(pybind11)
FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 8.1.1
)
FetchContent_MakeAvailable(fmt)
find_package(Eigen3 REQUIRED)
if (ENABLE_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()


set(doomtime_src ${CMAKE_CURRENT_SOURCE_DIR}/apps/doomtime/src)
set(doomtime_scripts ${CMAKE_CURRENT_SOURCE_DIR}/apps/doomtime/scripts)

if (ENABLE_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()

add_subdirectory(host)
