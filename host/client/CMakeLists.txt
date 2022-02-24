find_package(OpenCV REQUIRED COMPONENTS opencv_core)

add_library(client_impl
	frame_buffer.cpp
	transport.cpp
    consumer_thread.cpp
)
set_target_properties(client_impl PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(client_impl PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(client_impl PUBLIC
	doomtime_ipc_headers
    opencv_core
    atomic
    Eigen3::Eigen
    fmt::fmt-header-only
)

pybind11_add_module(doomtime_client client.cpp)
target_link_libraries(doomtime_client PRIVATE
    client_impl
    fmt::fmt-header-only
)
configure_file(client.py ${CMAKE_CURRENT_BINARY_DIR}/client.py COPYONLY)
