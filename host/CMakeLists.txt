cmake_minimum_required(VERSION 3.18)
set(CMAKE_CXX_STANDARD 17)


add_subdirectory(ipc_shared_data)
add_subdirectory(ipc_lib)
add_subdirectory(client)
if (ENABLE_TESTING)
    set(producer_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/ipc_lib)
    add_subdirectory(test)
endif()
