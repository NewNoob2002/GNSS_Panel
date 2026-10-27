add_library(hc32_easylogger_headers INTERFACE)
add_library(hc32::easylogger_headers ALIAS hc32_easylogger_headers)
target_include_directories(hc32_easylogger_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/inc)

if(NOT HC32_LOG_ENABLED)
    return()
endif()

add_library(hc32_easylogger OBJECT
    src/elog.c
    src/elog_async.c
    src/elog_buf.c
    src/elog_utils.c
    port/elog_port.c
)
add_library(hc32::easylogger ALIAS hc32_easylogger)
target_link_libraries(hc32_easylogger PUBLIC hc32::easylogger_headers proj::options
    PRIVATE hc32::easylogger_port)
