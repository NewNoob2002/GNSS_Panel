add_library(hc32_libraries_headers INTERFACE)
target_include_directories(hc32_libraries_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(Bq40z50)
add_subdirectory(MillisTaskManager)
add_subdirectory(Mp2762a)
add_subdirectory(Parser)
add_subdirectory(SEGGER_RTT)
add_subdirectory(easylogger)

# 三产品共用的设备驱动与工具库，保留原对象文件的汇总顺序。
add_library(hc32_libraries INTERFACE)
target_sources(hc32_libraries INTERFACE
    $<TARGET_OBJECTS:hc32::bq40z50>
    $<TARGET_OBJECTS:hc32::millis_task>
    $<TARGET_OBJECTS:hc32::mp2762a>
    $<TARGET_OBJECTS:hc32::parser>
    $<TARGET_OBJECTS:hc32::segger_rtt>
    $<TARGET_OBJECTS:hc32::easylogger>
)
target_link_libraries(hc32_libraries INTERFACE
    hc32_libraries_headers
    hc32::bq40z50
    hc32::millis_task
    hc32::mp2762a
    hc32::parser
    hc32::segger_rtt
    hc32::easylogger
)

if(NOT PRODUCT_HAS_DISPLAY)
    add_subdirectory(MultiButton)
endif()
