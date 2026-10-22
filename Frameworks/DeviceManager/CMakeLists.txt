add_library(fw_devicemanager OBJECT DeviceManager.cpp)
add_library(fw::devicemanager ALIAS fw_devicemanager)
target_include_directories(fw_devicemanager PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(fw_devicemanager PUBLIC proj::options PRIVATE app::log)
target_compile_options(fw_devicemanager PRIVATE -Wall -Wextra -Werror
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>)
