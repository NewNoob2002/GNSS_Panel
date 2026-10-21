cmake_minimum_required(VERSION 3.20)

project(HC32F460xE_Arduino LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 同一 MCU 的三个产品共用平台层与通信层，每次配置只构建一个产品。
set(HC32_PRODUCTS X1 X1Pro XBase)
set(PRODUCT "X1Pro" CACHE STRING "Firmware product to build: X1, X1Pro or XBase")
set_property(CACHE PRODUCT PROPERTY STRINGS ${HC32_PRODUCTS})
if(NOT PRODUCT IN_LIST HC32_PRODUCTS)
    message(FATAL_ERROR "Unknown PRODUCT '${PRODUCT}', expected one of: ${HC32_PRODUCTS}")
endif()
set(PRODUCT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Products/${PRODUCT})
include(${PRODUCT_DIR}/product.cmake)
message(STATUS "Product: ${PRODUCT} ${PRODUCT_VERSION} (display: ${PRODUCT_HAS_DISPLAY})")

set(PROJECT_VERSION ${PRODUCT_VERSION})
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Config/mcu_version_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/Platform/Config/mcu_version_config.h
    @ONLY
)
set(TARGET_NAME ${PRODUCT_IMAGE_PREFIX}_${PRODUCT_VERSION})
set(CMAKE_EXECUTABLE_SUFFIX .elf)
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/HC32F460xE.ld)

include(cmake/ProjectOptions.cmake)

add_subdirectory(Platform)
add_subdirectory(Arduino)
add_subdirectory(Libraries)
if(PRODUCT_HAS_DISPLAY)
    add_library(app_build_options INTERFACE)
    target_link_libraries(app_build_options INTERFACE hc32_project_options)
    add_library(app_lvgl INTERFACE)
    target_link_libraries(app_lvgl INTERFACE hc32_display_libraries)
    add_subdirectory(Shared/App)
endif()
add_subdirectory(Shared)
add_subdirectory(${PRODUCT_DIR})

add_executable(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} PRIVATE
    hc32_platform
    hc32_arduino
    hc32_libraries
    hc32_shared_comm
    hc32_product
)
target_link_options(${TARGET_NAME} PRIVATE
    ${CPU_FLAGS}
    -T${LINKER_SCRIPT}
    -Wl,--defsym=FLASH_ORIGIN=0x00008000
    -Wl,--defsym=FLASH_LENGTH=0x00072000
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.map
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    --specs=nano.specs
    --specs=nosys.specs
)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${TARGET_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/generated/${TARGET_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/generated/${TARGET_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET_NAME}>
    COMMENT "Generating HEX/BIN and size report"
)
set_property(TARGET ${TARGET_NAME} APPEND PROPERTY LINK_DEPENDS ${LINKER_SCRIPT})
