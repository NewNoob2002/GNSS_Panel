add_library(fw_databroker OBJECT DataNode.cpp DataBroker.cpp DataTimer.cpp)
add_library(fw::databroker ALIAS fw_databroker)
# 3.3b 保持原 app_framework 的包含路径与编译选项；可见性收紧留给 3.3c。
get_filename_component(APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../App" ABSOLUTE)
target_include_directories(fw_databroker PUBLIC "${APP_DIR}" "${APP_DIR}/..")
target_link_libraries(fw_databroker PUBLIC app_build_options PRIVATE app_log)
target_compile_options(fw_databroker PRIVATE -Wall -Wextra -Werror
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>)
