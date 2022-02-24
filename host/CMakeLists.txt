cmake_minimum_required(VERSION 3.18)
project(doomtime_host)

set(CMAKE_CXX_STANDARD 17)
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

add_subdirectory(ipc_shared_data)
add_subdirectory(ipc_lib)
add_subdirectory(client)
if (ENABLE_TESTING)
    enable_testing()
    set(producer_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/ipc_lib)
    add_subdirectory(test)
endif()
