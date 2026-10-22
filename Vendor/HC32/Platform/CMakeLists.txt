add_library(hc32_platform_headers INTERFACE)
target_include_directories(hc32_platform_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Device/HDSC/hc32f4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Config
    ${CMAKE_CURRENT_SOURCE_DIR}/DDL/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/HAL
    ${CMAKE_CURRENT_SOURCE_DIR}/Peripherals
    ${CMAKE_CURRENT_SOURCE_DIR}/State
    ${CMAKE_CURRENT_SOURCE_DIR}/System
)
target_link_libraries(hc32_platform_headers INTERFACE hc32_project_options hc32::easylogger_headers)

file(GLOB PLATFORM_PERIPHERAL_SOURCES CONFIGURE_DEPENDS
    Peripherals/*.c
    Peripherals/*.cpp
)
# SpiMaster 当前只有显示产品使用，保留平台目标归属；X1 不编译显示 SPI 代码。
if(NOT PRODUCT_HAS_DISPLAY)
    list(FILTER PLATFORM_PERIPHERAL_SOURCES EXCLUDE REGEX "SpiMaster\\.cpp$")
endif()
file(GLOB PLATFORM_SYSTEM_SOURCES CONFIGURE_DEPENDS
    System/*.c
    System/*.cpp
)
file(GLOB PLATFORM_CONFIG_SOURCES CONFIGURE_DEPENDS
    Config/*.cpp
)
file(GLOB PLATFORM_HAL_SOURCES CONFIGURE_DEPENDS
    HAL/*.c
    HAL/*.cpp
)
# 系统状态分域与写入口（纯 C++，主机测试与模拟器也直接编译）
file(GLOB PLATFORM_STATE_SOURCES CONFIGURE_DEPENDS
    State/*.cpp
)
file(GLOB PLATFORM_DDL_SOURCES CONFIGURE_DEPENDS
    DDL/src/*.c
)
add_library(hc32_platform OBJECT
    CMSIS/Device/HDSC/hc32f4xx/Source/GCC/startup_hc32f460.S
    CMSIS/Device/HDSC/hc32f4xx/Source/system_hc32f460.c
    ${PLATFORM_PERIPHERAL_SOURCES}
    ${PLATFORM_SYSTEM_SOURCES}
    ${PLATFORM_CONFIG_SOURCES}
    ${PLATFORM_HAL_SOURCES}
    ${PLATFORM_STATE_SOURCES}
    ${PLATFORM_DDL_SOURCES}
)

target_link_libraries(hc32_platform PUBLIC
    hc32_platform_headers
)
# 平台层三产品共用，不依赖显示库或界面代码。
target_include_directories(hc32_platform PRIVATE
    ${PROJECT_SOURCE_DIR}/Libraries
    ${PROJECT_SOURCE_DIR}/Libraries/Bq40z50
    ${PROJECT_SOURCE_DIR}/Libraries/MillisTaskManager
    ${PROJECT_SOURCE_DIR}/Libraries/Mp2762a
    ${PROJECT_SOURCE_DIR}/Libraries/SEGGER_RTT
    ${PROJECT_SOURCE_DIR}/Libraries/easylogger/inc
)
