include(FetchContent)
FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.11.0
)
FetchContent_MakeAvailable(googletest)

add_executable(test_frame_buffer test_frame_buffer.cpp)
target_link_libraries(test_frame_buffer PRIVATE
	GTest::gtest
	GTest::gtest_main
	client_impl
)

pybind11_add_module(doomtime_producer producer.cpp)
target_link_libraries(doomtime_producer PRIVATE
    doomtime_ipc
    Eigen3::Eigen
)
target_include_directories(doomtime_producer PRIVATE ${producer_include_dir})

add_test(NAME test_without_bluez
    COMMAND ${Python3_EXECUTABLE} -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/test_without_bluez.py)
set_tests_properties(test_without_bluez PROPERTIES ENVIRONMENT
    PYTHONPATH=$<SHELL_PATH:$<TARGET_FILE_DIR:doomtime_producer>$<SEMICOLON>$<TARGET_FILE_DIR:doomtime_client>>
)

add_test(NAME test_frame_buffer COMMAND test_frame_buffer)
