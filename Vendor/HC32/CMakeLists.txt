cmake_minimum_required(VERSION 3.20)
project(GNSS_Panel LANGUAGES C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." REALPATH)
set(HC32_PRODUCTS X1 X1Pro XBase)
set(PRODUCT "X1Pro" CACHE STRING "Firmware product to build: X1, X1Pro or XBase")
set_property(CACHE PRODUCT PROPERTY STRINGS ${HC32_PRODUCTS})
if(NOT PRODUCT IN_LIST HC32_PRODUCTS)
    message(FATAL_ERROR "Unknown PRODUCT '${PRODUCT}'")
endif()
set(PRODUCT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Products/${PRODUCT})
include(${PRODUCT_DIR}/product.cmake)
message(STATUS "Product: ${PRODUCT} ${PRODUCT_VERSION} (display: ${PRODUCT_HAS_DISPLAY})")
set(PROJECT_VERSION ${PRODUCT_VERSION})
configure_file(Platform/Config/mcu_version_config.h.in
    ${CMAKE_BINARY_DIR}/generated/Platform/Config/mcu_version_config.h @ONLY)
set(TARGET_NAME ${PRODUCT_IMAGE_PREFIX}_${PRODUCT_VERSION})
set(CMAKE_EXECUTABLE_SUFFIX .elf)
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/HC32F460xE.ld)
include(${REPO_ROOT}/cmake/ProjectOptions.cmake)
add_subdirectory(Libraries)
add_subdirectory(Platform)
if(NOT PRODUCT_HAS_DISPLAY)
    include(${REPO_ROOT}/App/oem.cmake)
endif()
if(PRODUCT_HAS_DISPLAY)
    include(${REPO_ROOT}/cmake/lvgl.cmake)
    project_add_lvgl("${CMAKE_CURRENT_SOURCE_DIR}/Display/Lvgl/lv_conf.h")
    target_link_libraries(lvgl PUBLIC proj::options)
    target_compile_options(lvgl PRIVATE
        $<TARGET_PROPERTY:proj::options,INTERFACE_COMPILE_OPTIONS>
        $<$<CONFIG:Debug>:-Os>)
    add_subdirectory(${REPO_ROOT}/App ${CMAKE_CURRENT_BINARY_DIR}/App)
    set(display_arg DISPLAY)
endif()
include(cmake/portable.cmake)
hc32_add_portable("" ${PRODUCT} hc32::hardware_port FIRMWARE ${display_arg})
add_subdirectory(${PRODUCT_DIR})
add_executable(${TARGET_NAME})
add_executable(hc32::firmware ALIAS ${TARGET_NAME})
set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# 产品层显式保留重组前的对象顺序，避免链接布局改变固件体积。
include(cmake/firmware_objects.cmake)
hc32_firmware_objects(${TARGET_NAME})
string(TOLOWER ${PRODUCT} product_name)
if(PRODUCT_HAS_DISPLAY)
    target_link_libraries(${TARGET_NAME} PRIVATE hc32::product_${product_name})
endif()
target_link_options(${TARGET_NAME} PRIVATE
    ${CPU_FLAGS} -T${LINKER_SCRIPT}
    -Wl,--defsym=FLASH_ORIGIN=0x00008000 -Wl,--defsym=FLASH_LENGTH=0x00072000
    -Wl,-Map=${CMAKE_BINARY_DIR}/${TARGET_NAME}.map
    -Wl,--gc-sections -Wl,--print-memory-usage --specs=nano.specs --specs=nosys.specs)
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${TARGET_NAME}> ${CMAKE_BINARY_DIR}/generated/${TARGET_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET_NAME}> ${CMAKE_BINARY_DIR}/generated/${TARGET_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET_NAME}>
    COMMENT "Generating HEX/BIN and size report")
set_property(TARGET ${TARGET_NAME} APPEND PROPERTY LINK_DEPENDS ${LINKER_SCRIPT})
