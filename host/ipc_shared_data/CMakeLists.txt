find_package(Boost REQUIRED COMPONENTS headers)
find_package(Threads REQUIRED)
add_library(doomtime_ipc_headers INTERFACE)
target_include_directories(doomtime_ipc_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(doomtime_ipc_headers INTERFACE
    Boost::headers
    -lrt
    Threads::Threads
)
