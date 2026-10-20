add_library(hc32_platform_headers INTERFACE)
target_include_directories(hc32_platform_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Device/HDSC/hc32f4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Config
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/HAL
    ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(hc32_platform_headers INTERFACE hc32_project_options)

file(GLOB PLATFORM_CORE_SOURCES CONFIGURE_DEPENDS
    Core/src/*.c
    Core/src/*.cpp
)
file(GLOB PLATFORM_HAL_SOURCES CONFIGURE_DEPENDS
    HAL/*.c
    HAL/*.cpp
)
file(GLOB PLATFORM_DRIVER_SOURCES CONFIGURE_DEPENDS
    Drivers/src/*.c
)
file(GLOB PLATFORM_SUPPORT_SOURCES CONFIGURE_DEPENDS
    support/*.c
)

list(FILTER PLATFORM_DRIVER_SOURCES EXCLUDE REGEX "hc32_ll_icg\\.c$")

add_library(hc32_platform OBJECT
    CMSIS/Device/HDSC/hc32f4xx/Source/GCC/startup_hc32f460.S
    CMSIS/Device/HDSC/hc32f4xx/Source/system_hc32f460.c
    ${PLATFORM_CORE_SOURCES}
    ${PLATFORM_HAL_SOURCES}
    ${PLATFORM_DRIVER_SOURCES}
    ${PLATFORM_SUPPORT_SOURCES}
)

target_link_libraries(hc32_platform PUBLIC
    hc32_platform_headers
)
# 平台层三产品共用，不依赖显示库或界面代码。
target_include_directories(hc32_platform PRIVATE
    ${PROJECT_SOURCE_DIR}/Arduino
    ${PROJECT_SOURCE_DIR}/Arduino/Core/avr
    ${PROJECT_SOURCE_DIR}/Arduino/Core/variants/hc32f460xx
    ${PROJECT_SOURCE_DIR}/Libraries
    ${PROJECT_SOURCE_DIR}/Libraries/Bq40z50
    ${PROJECT_SOURCE_DIR}/Libraries/MillisTaskManager
    ${PROJECT_SOURCE_DIR}/Libraries/Mp2762a
    ${PROJECT_SOURCE_DIR}/Libraries/easylogger/inc
)
