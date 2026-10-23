# 芯片、时基、HAL 与日志的声明端口不携带实现，避免反向链接。
add_library(hc32_hardware_port INTERFACE)
add_library(hc32::hardware_port ALIAS hc32_hardware_port)
target_include_directories(hc32_hardware_port INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/HAL
    ${CMAKE_CURRENT_SOURCE_DIR}/Peripherals
    ${CMAKE_CURRENT_SOURCE_DIR}/System)
target_link_libraries(hc32_hardware_port INTERFACE proj::options
    hc32::cmsis_headers hc32::ddl_headers hc32::config_headers hc32::easylogger_headers)
add_subdirectory(CMSIS)
add_subdirectory(DDL)
set(HC32_PORT hc32::hardware_port)
add_subdirectory(System)
