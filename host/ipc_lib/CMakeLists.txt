add_library(doomtime_ipc SHARED producer.cpp)
set_target_properties(doomtime_ipc PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(doomtime_ipc PUBLIC doomtime_ipc_headers)
target_link_libraries(doomtime_ipc PRIVATE fmt::fmt-header-only)
