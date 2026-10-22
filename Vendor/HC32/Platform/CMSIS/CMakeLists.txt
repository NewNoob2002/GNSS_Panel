add_library(hc32_cmsis_headers INTERFACE)
add_library(hc32::cmsis_headers ALIAS hc32_cmsis_headers)
target_include_directories(hc32_cmsis_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Device/HDSC/hc32f4xx/Include)
target_link_libraries(hc32_cmsis_headers INTERFACE proj::options)

add_library(hc32_cmsis OBJECT
    Device/HDSC/hc32f4xx/Source/GCC/startup_hc32f460.S
    Device/HDSC/hc32f4xx/Source/system_hc32f460.c)
add_library(hc32::cmsis ALIAS hc32_cmsis)
target_link_libraries(hc32_cmsis PUBLIC hc32::cmsis_headers
    PRIVATE hc32::ddl_headers hc32::config_headers)
