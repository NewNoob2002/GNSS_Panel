add_library(hc32_millis_task_manager OBJECT MillisTaskManager.cpp)
add_library(hc32::millis_task ALIAS hc32_millis_task_manager)
target_include_directories(hc32_millis_task_manager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hc32_millis_task_manager PUBLIC proj::options PRIVATE hc32::millis_task_port)
